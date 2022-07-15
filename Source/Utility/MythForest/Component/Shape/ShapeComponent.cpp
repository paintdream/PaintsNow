#include "ShapeComponent.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <queue>

using namespace PaintsNow;

ShapeComponent::ShapeComponent() {}
ShapeComponent::~ShapeComponent() {
	Cleanup();
}

class CodeIndex {
public:
	CodeIndex(uint32_t c = 0, uint32_t i = 0) : code(c), index(i) {}
	bool operator < (const CodeIndex& rhs) const {
		return code < rhs.code;
	}

	uint32_t code;
	uint32_t index;
};

// Generate Z-code
static inline CodeIndex Encode(const UShort3Pair& box, uint32_t index) {
	return CodeIndex(Math::Interleave(UniqueType<uint32_t>(), &box.first.data[0], 6u), index);
}

void ShapeComponent::MakeHeapInternal(std::vector<Patch>& output, Patch* begin, Patch* end, uint8_t index) {
	if (begin < end) {
		Patch* mid = begin + (end - begin) / 2;
		mid->SetIndex(index);

		index = (index + 1) % 6;

		output.emplace_back(std::move(*mid));
		MakeHeapInternal(output, begin, mid, index);
		MakeHeapInternal(output, mid + 1, end, index);
	}
}

ShapeComponent::Patch* ShapeComponent::MakeBound(Patch& patch, const std::vector<Float3>& vertices, const std::vector<UInt3>& indices) {
	Float3Pair box;
	for (uint32_t i = 0; i < MAX_PATCH_COUNT; i++) {
		uint32_t index = patch.indices[i];
		if (index == ~(uint32_t)0) {
			assert(i != 0);
			break;
		}

		const UInt3& face = indices[index];

		for (size_t t = 0; t < 3; t++) {
			const Float3& v = vertices[face[t]];

			for (size_t m = 0; m < 3; m++) {
				if (i == 0 && t == 0 && m == 0) {
					box = Float3Pair(v, v);
				} else {
					Math::Union(box, v);
				}

				patch.vertices[i / 4][t][m][i & 3] = v[m];
			}
		}
	}

	patch.GetKey() = box;
	return &patch;
}

void ShapeComponent::Cleanup() {
	if (meshResource) {
		meshResource->UnMap();
		meshResource = nullptr;
	}
}

void ShapeComponent::Update(Engine& engine, const TShared<MeshResource>& resource) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_PINNED));
	if (!(Flag().fetch_or(TINY_UPDATING, std::memory_order_acquire) & TINY_UPDATING)) {
		engine.GetKernel().GetThreadPool().Dispatch(CreateTaskContextFree(Wrap(this, &ShapeComponent::RoutineUpdate), std::ref(engine), resource), 1);
	}
}

void ShapeComponent::CheckBounding(Patch* root, Float3Pair& targetKey) {
#ifdef _DEBUG
	// ranged queryer
	Float3Pair entry = targetKey;
	for (Patch* p = root; p != nullptr; p = (Patch*)(p->GetRight())) {
		assert(Math::Overlap(p->GetKey(), targetKey));

		// culling
		Float3Pair before = targetKey;
		int index = p->GetIndex();
		float save = Patch::Meta::SplitPush(std::true_type(), targetKey, p->GetKey(), index);
		// cull right in left node
		if (p->GetLeft() != nullptr) {
			CheckBounding(p->GetLeft(), targetKey);
		}

		Patch::Meta::SplitPop(targetKey, index, save);
	}
#endif
}

void ShapeComponent::RoutineUpdate(Engine& engine, const TShared<MeshResource>& resource) {
	if (resource == meshResource)
		return;

	OPTICK_EVENT();

	SnowyStream& snowyStream = engine.snowyStream;
	if (resource->Map()) {
		Cleanup();
		meshResource = resource;

		IAsset::MeshCollection& meshCollection = meshResource->meshCollection;
		Float3Pair bound = meshResource->boundingBox;

		// Build Tree
		const std::vector<Float3>& vertices = meshCollection.vertices;
		const std::vector<UInt3>& indices = meshCollection.indices;

		assert(!vertices.empty() && !indices.empty());

		// safe
		for (uint32_t n = 0; n < 3; n++) {
			if (bound.second[n] - bound.first[n] < 1e-4f) {
				bound.second[n] = bound.first[n] + 1e-4f;
			}
		}

		// convert to local position
		uint32_t level = (uint32_t)sizeof(uint32_t) * 8 / 6;
		uint16_t div = 1 << (level + 1);
		UShort3 divCount(div - 1, div - 1, div - 1);

		std::vector<CodeIndex> codeIndices;
		codeIndices.reserve(indices.size());
		for (uint32_t i = 0; i < indices.size(); i++) {
			const UInt3& index = indices[i];
			UShort3 first = Math::QuantizeVector(bound, vertices[index.x()], divCount);
			UShort3Pair box(first, first);
			Math::Union(box, Math::QuantizeVector(bound, vertices[index.y()], divCount));
			Math::Union(box, Math::QuantizeVector(bound, vertices[index.z()], divCount));
			codeIndices.emplace_back(Encode(box, i));
		}

		// sort by z-code
		std::sort(codeIndices.begin(), codeIndices.end());

		// make tree
		std::vector<Patch> linearPatches;

		if (!codeIndices.empty()) {
			uint32_t k = 0;
			linearPatches.reserve((codeIndices.size() + MAX_PATCH_COUNT - 1) / MAX_PATCH_COUNT);
			linearPatches.emplace_back(Patch());

			for (uint32_t m = 0; m < codeIndices.size(); m++) {
				const CodeIndex& codeIndex = codeIndices[m];
				if (k == MAX_PATCH_COUNT) {
					k = 0;
					linearPatches.emplace_back(Patch());
				}

				linearPatches.back().indices[k++] = codeIndex.index;
			}

			while (k != MAX_PATCH_COUNT) {
				linearPatches.back().indices[k++] = ~(uint32_t)0;
			}
		}

		// heap order
		std::vector<Patch> newPatches;
		newPatches.reserve(linearPatches.size());
		MakeHeapInternal(newPatches, &linearPatches[0], &linearPatches[0] + linearPatches.size(), 0);

		// Connect
		Patch* root = MakeBound(newPatches[0], vertices, indices);
		for (uint32_t s = 1; s < newPatches.size(); s++) {
			root->Attach(MakeBound(newPatches[s], vertices, indices));
		}

		CheckBounding(root, bound);

		std::swap(newPatches, patches);
		boundingBox = bound;
	} else {
		resource->UnMap();
		assert(false);
	}

	Flag().fetch_or(TINY_PINNED, std::memory_order_relaxed);
	Flag().fetch_and(~TINY_UPDATING, std::memory_order_release);
}

class ShapeComponent::PatchRayCuller {
public:
	PatchRayCuller(const Float3Pair& r) : quickRay(Math::QuickRay(r)) {}
	Float3Pair quickRay;
	float lastDistance;

	bool operator () (const Float3Pair& box) {
		lastDistance = Math::IntersectBox(box, quickRay);
		return lastDistance >= 0.0f;
	}
};


class ShapeComponent::PatchRayCaster {
public:
	typedef TVector<TVector<float, 4>, 3> Group;

	class DeferredBox {
	public:
		DeferredBox(const Patch* n, float d) : node(n), nearestDistance(d) {}

		bool operator < (const DeferredBox& rhs) const {
			return nearestDistance > rhs.nearestDistance;
		}

		const Patch* node;
		float nearestDistance;
	};

	typedef TCacheAllocator<DeferredBox> DeferredAllocator;

	PatchRayCaster(PatchRayCuller& c, const std::vector<Float3>& v, const std::vector<UInt3>& i, const Float3Pair& r, std::priority_queue<DeferredBox, std::vector<DeferredBox, DeferredAllocator> >* boxes) : culler(c), vertices(v), indices(i), ray(r), hitPatch(nullptr), hitIndex(0), distance(FLT_MAX), deferredBoxes(boxes) {
		Math::ExtendVector(rayGroup.first, TVector<float, 3>(r.first));
		Math::ExtendVector(rayGroup.second, TVector<float, 3>(r.second));
	}

	void ProcessPatch(const Patch& patch) {
		TVector<TVector<float, 4>, 3> res;
		TVector<TVector<float, 4>, 2> uv;

		static_assert(MAX_PATCH_COUNT % 4 == 0, "Must be 4n size");
		for (uint32_t k = 0; k < MAX_PATCH_COUNT / 4; k++) {
			Math::IntersectTriangle(res, uv, patch.vertices[k], rayGroup);
			for (uint32_t m = 0; m < 4; m++) {
				if (patch.indices[k * 4 + m] == ~(uint32_t)0)
					return;

				if (uv[0][m] >= 0.0f && uv[1][m] >= 0.0f && uv[0][m] + uv[1][m] <= 1.0f) {
					Float3 hit(res[0][m], res[1][m], res[2][m]);
					Float3 vec = hit - ray.first;
					if (Math::DotProduct(vec, ray.second) > 0) {
						float s = Math::SquareLength(vec);
						if (s < distance) {
							distance = s;
							hitPatch = &patch;
							hitIndex = patch.indices[k * 4 + m];
							coord = Float4(uv[0][m], uv[1][m], 0, 0);
							intersection = hit;
						}
					}
				}
			}
		}
	}

	bool operator () (const TKdTree<Float3Pair, VertexStorage>& node) {
		// use cache?
		const Patch& patch = static_cast<const Patch&>(node);
		if (deferredBoxes != nullptr) {
			deferredBoxes->push(DeferredBox(&patch, culler.lastDistance));
		} else {
			IMemory::PrefetchReadLocal(patch.GetLeft());
			if /*constexpr*/ (sizeof(Patch) > CPU_CACHELINE_SIZE) {
				IMemory::PrefetchReadLocal(reinterpret_cast<uint8_t*>(patch.GetLeft()) + CPU_CACHELINE_SIZE);
			}
			IMemory::PrefetchReadLocal(patch.GetRight());
			if /*constexpr*/ (sizeof(Patch) > CPU_CACHELINE_SIZE) {
				IMemory::PrefetchReadLocal(reinterpret_cast<uint8_t*>(patch.GetRight()) + CPU_CACHELINE_SIZE);
			}

			ProcessPatch(patch);
		}

		return true;
	}

	void Resolve() {
		if (deferredBoxes != nullptr) {
			while (!deferredBoxes->empty()) {
				const DeferredBox& box = deferredBoxes->top();
				if (Math::SquareLength(ray.second * box.nearestDistance) >= distance) { // no better intersections jump out
					break;
				}

				ProcessPatch(*box.node);
				deferredBoxes->pop();
			}
		}
	}

	PatchRayCuller& culler;
	std::priority_queue<DeferredBox, std::vector<DeferredBox, DeferredAllocator> >* deferredBoxes;
	const std::vector<Float3>& vertices;
	const std::vector<UInt3>& indices;
	std::pair<Group, Group> rayGroup;
	const Patch* hitPatch;
	uint32_t hitIndex;
	const Float3Pair& ray;
	Float3 intersection;
	float distance;
	Float4 coord;
};

void ShapeComponent::Complete(RaycastTask& task, Float3Pair& ray, const MatrixFloat4x4& transform, Unit* parent, float ratio, void* context) const {
	Float3Pair box = boundingBox;
	IAsset::MeshCollection& meshCollection = meshResource->meshCollection;

	PatchRayCuller c(ray);
	PatchRayCaster q(c, meshCollection.vertices, meshCollection.indices, ray, reinterpret_cast<std::priority_queue<PatchRayCaster::DeferredBox, std::vector<PatchRayCaster::DeferredBox, PatchRayCaster::DeferredAllocator> >*>(context));
	(const_cast<Patch&>(patches[0])).Query(std::true_type(), box, q, c);
	q.Resolve();

	if (q.hitPatch != nullptr) {
		RaycastResult result;
		result.transform = transform;
		result.position = q.intersection;
		result.squareDistance = q.distance * ratio;
		result.faceIndex = q.hitIndex;
		result.coord = q.coord;
		result.unit = const_cast<ShapeComponent*>(this);
		result.parent = parent;
		task.EmplaceResult(std::move(result));
	}
}

float ShapeComponent::Raycast(RaycastTask& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio) const {
	if (Flag().load(std::memory_order_acquire) & TINY_PINNED) {
		if (!patches.empty()) {
			OPTICK_EVENT();
			if (task.cache != nullptr) {
				PatchRayCaster::DeferredAllocator deferredAllocator(task.cache);
				std::priority_queue<PatchRayCaster::DeferredBox, std::vector<PatchRayCaster::DeferredBox, PatchRayCaster::DeferredAllocator> > deferredBoxes(std::less<PatchRayCaster::DeferredBox>(), deferredAllocator);

				Complete(task, ray, transform, parent, ratio, &deferredBoxes);
			} else {
				Complete(task, ray, transform, parent, ratio, nullptr);
			}
		}

		return ratio;
	} else {
		return 0.0f;
	}
}

const TShared<MeshResource>& ShapeComponent::GetMesh() const {
	return meshResource;
}

