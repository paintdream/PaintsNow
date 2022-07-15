#include "TerrainResource.h"

#include <cmath>

using namespace PaintsNow;

TerrainResource::TerrainResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), width(0), height(0), deltaHeight(0.05f) {}

void TerrainResource::Attach(IRender& render, void* deviceContext) {
}

void TerrainResource::Detach(IRender& render, void* deviceContext) {

}

void TerrainResource::Upload(IRender& render, void* deviceContext) {

}

void TerrainResource::Download(IRender& render, void* deviceContext) {

}

void TerrainResource::FromTexture(const TShared<TextureResource>& textureResource) {
	textureResource->Map();

	IRender::Resource::TextureDescription& description = textureResource->description;
	// Check texture type
	width = description.dimension.x();
	height = description.dimension.y();
	terrainData.resize(width * height);
	assert(description.state.layout == IRender::Resource::TextureDescription::R);

	if (description.state.format == IRender::Resource::TextureDescription::FLOAT) {
		memcpy(&terrainData[0], description.data.GetData(), width * height * sizeof(terrainData[0]));
	} else if (description.state.format == IRender::Resource::TextureDescription::UNSIGNED_SHORT) {
		const uint16_t* p = reinterpret_cast<const uint16_t*>(description.data.GetData());
		for (size_t i = 0; i < width * height; i++) {
			terrainData[i] = (float)p[i] / 0xFFFF;
		}
	} else if (description.state.format == IRender::Resource::TextureDescription::UNSIGNED_BYTE) {
		const uint16_t* p = reinterpret_cast<const uint16_t*>(description.data.GetData());
		for (size_t i = 0; i < width * height; i++) {
			terrainData[i] = (float)p[i] / 0xFF;
		}
	} else {
		assert(false); // not supported
	}

	textureResource->UnMap();
}

TObject<IReflect>& TerrainResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(terrainData);
		ReflectProperty(width);
		ReflectProperty(height);
		ReflectProperty(deltaHeight);
	}

	return *this;
}

// Computations

class GradVertex {
public:
	GradVertex() {}
	GradVertex(const Float3& v, const Float2& g) : vertex(v), grad(g) {}
	static GradVertex Mix(const GradVertex& lhs, const GradVertex& rhs, float v) {
		return GradVertex(Math::Interpolate(lhs.vertex, rhs.vertex, v), Math::Interpolate(lhs.grad, rhs.grad, v));
	}

	Float3 vertex;
	Float2 grad;
};

inline uint32_t ToCacheIndex(uint32_t i, uint32_t j, uint32_t M, uint32_t N) {
	return i + j * M;
}

inline bool InRange(const Float2& center, const Float2& sigma, uint32_t i, uint32_t j) {
	return Math::SquareLength(Float2((center.x() - i) / sigma.x(), (center.y() - j) / sigma.y())) <= 1;
}

inline void FixRange(uint32_t& i, uint32_t& j, uint32_t totalWidth, uint32_t totalHeight) {
	if ((int32_t)i < 0) {
		i = 0;
	} else if (i >= totalWidth) {
		i = totalWidth - 1;
	}

	if ((int32_t)j < 0) {
		j = 0;
	} else if (j >= totalHeight) {
		j = totalHeight - 1;
	}
}

inline const Float3& Get(uint32_t i, uint32_t j, std::vector<Float3>& gradHeightMap, uint32_t totalWidth, uint32_t totalHeight) {
	return gradHeightMap[i + j * totalWidth];
}

inline uint32_t GenVertex(std::vector<GradVertex>& buffer, const GradVertex& desc, bool forceNew = false) {
	const float EPSILON = 0.01f;
	if (forceNew || buffer.size() == 0 || Math::SquareLength(buffer[buffer.size() - 1].vertex - desc.vertex) > EPSILON) {
		buffer.emplace_back(desc);
	}

	return verify_cast<uint32_t>(buffer.size() - 1);
}

inline uint32_t GetVertex(std::vector<GradVertex>& buffer, uint32_t i, uint32_t j, std::vector<Float3>& gradHeightMap, uint32_t totalWidth, uint32_t totalHeight, bool forceNew) {
	const Float3& g = Get(i, j, gradHeightMap, totalWidth, totalHeight);
	GradVertex v(Float3((float)i / (totalWidth - 1), (float)j / (totalHeight - 1), g.z()), Float2(g.x(), g.y()));
	return GenVertex(buffer, v, forceNew);
}

template <bool x>
inline GradVertex Hermite(const GradVertex& u, const GradVertex& v, float t) {
	GradVertex res;
	// res.vertex = u.vertex * (1 - t) + v.vertex * t;
	res.grad = u.grad * (1 - t) + v.grad * t;

	res.vertex.x() = (1 - t) * u.vertex.x() + t * v.vertex.x();
	res.vertex.y() = (1 - t) * u.vertex.y() + t * v.vertex.y();
	res.vertex.z() = (1 - t) * (1 - t) * ((1 + 2 * t) * u.vertex.z() + t * (x ? u.grad.x() : u.grad.y())) + (t * t) * ((3 - 2 * t) * v.vertex.z() + (t - 1) * (x ? v.grad.x() : v.grad.y()));
	return res;
}

inline GradVertex HermiteDouble(const GradVertex& leftTop, const GradVertex& rightTop, const GradVertex& leftBottom, const GradVertex& rightBottom, float u, float v) {
	GradVertex start = Hermite<true>(leftTop, rightTop, u);
	GradVertex end = Hermite<true>(leftBottom, rightBottom, u);
	return Hermite<false>(start, end, v);
}

inline GradVertex Lerp(const GradVertex& first, const GradVertex& next, bool x) {
	return x ? Hermite<true>(first, next, 0.5) : Hermite<false>(first, next, 0.5);
}

inline uint32_t GenLerpVertex(std::vector<GradVertex>& buffer, const GradVertex& first, const GradVertex& next, bool x) {
	return GenVertex(buffer, Lerp(first, next, x), false);
}

class Region {
public:
	Float3Pair box; // 24

	UInt2Pair range; // 16
	std::pair<uint32_t, uint32_t> faceRange; // 8
	std::pair<uint32_t, uint32_t> regionRange; // 8
};

inline bool IsGap(const GradVertex& lhs, const GradVertex& rhs, const GradVertex& res) {
	const float EPSILON = 0.001f;
	Float3 mid = (lhs.vertex + rhs.vertex) * 0.5f;
	return fabs(mid.z() - res.vertex.z()) > EPSILON;
}

inline void MakeTriangle(std::vector<Int3>& faces, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
	if (d != ~(uint32_t)0) {
		faces.emplace_back(Int3(a, b, d));
		faces.emplace_back(Int3(c, d, b));
	} else {
		faces.emplace_back(Int3(a, b, c));
	}
}

template <bool Detail>
void GenerateDetails(Region& root, std::vector<Region>& regions, std::vector<GradVertex>& buffer, std::vector<Int3>& faces, const UInt4& coords, const UInt2& size, float deltaHeight) {
	uint32_t leftTop = coords.x();
	uint32_t rightTop = coords.y();
	uint32_t leftBottom = coords.z();
	uint32_t rightBottom = coords.w();
	uint32_t totalWidth = size.x();
	uint32_t totalHeight = size.y();

	// hermite interpolation
	uint32_t mx = (root.range.first.x() + root.range.second.x()) >> 1;
	uint32_t my = (root.range.first.y() + root.range.second.y()) >> 1;
	uint32_t indexLeft = ToCacheIndex(root.range.first.x(), my, totalWidth, totalHeight);
	uint32_t indexRight = ToCacheIndex(root.range.second.x(), my, totalWidth, totalHeight);
	uint32_t indexTop = ToCacheIndex(mx, root.range.first.y(), totalWidth, totalHeight);
	uint32_t indexBottom = ToCacheIndex(mx, root.range.second.y(), totalWidth, totalHeight);

	GradVertex left = Detail ? Hermite<true>(buffer[leftTop], buffer[leftBottom], 0.5f) : buffer[indexLeft];
	GradVertex right = Detail ? Hermite<true>(buffer[rightTop], buffer[rightBottom], 0.5f) : buffer[indexRight];
	GradVertex top = Detail ? Hermite<false>(buffer[leftTop], buffer[rightTop], 0.5f) : buffer[indexTop];
	GradVertex bottom = Detail ? Hermite<false>(buffer[leftBottom], buffer[rightBottom], 0.5f) : buffer[indexBottom];

	GradVertex vXY = Hermite<false>(top, bottom, 0.5f);
	GradVertex vYX = Hermite<true>(left, right, 0.5f);

	GradVertex v = Detail ? GradVertex::Mix(vXY, vYX, 0.5f) : buffer[ToCacheIndex(mx, my, totalWidth, totalHeight)];

	bool leftGap = IsGap(buffer[leftTop], buffer[leftBottom], left);
	bool rightGap = IsGap(buffer[rightTop], buffer[rightBottom], right);
	bool topGap = IsGap(buffer[leftTop], buffer[rightTop], top);
	bool bottomGap = IsGap(buffer[leftBottom], buffer[rightBottom], bottom);

	int sum = (int)leftGap + (int)rightGap + (int)topGap + (int)bottomGap;

	uint32_t faceStart = verify_cast<uint32_t>(faces.size());

	float a = fabs(buffer[leftTop].vertex.z() + buffer[rightBottom].vertex.z() - 2 * v.vertex.z());
	float b = fabs(buffer[rightTop].vertex.z() + buffer[leftBottom].vertex.z() - 2 * v.vertex.z());

	indexLeft = leftGap ? Detail ? GenVertex(buffer, left) : indexLeft : ~(uint32_t)0;
	indexRight = rightGap ? Detail ? GenVertex(buffer, right) : indexRight : ~(uint32_t)0;
	indexTop = topGap ? Detail ? GenVertex(buffer, top) : indexTop : ~(uint32_t)0;
	indexBottom = bottomGap ? Detail ? GenVertex(buffer, bottom) : indexBottom : ~(uint32_t)0;
	uint32_t indexCenter = ~(uint32_t)0;

	if (sum >= 2) { // no opt. way
		indexCenter = Detail ? GenVertex(buffer, v, true) : ToCacheIndex(mx, my, totalWidth, totalHeight);
		MakeTriangle(faces, leftTop, indexCenter, leftBottom, indexLeft);
		MakeTriangle(faces, rightTop, indexCenter, leftTop, indexRight);
		MakeTriangle(faces, rightBottom, indexCenter, rightTop, indexTop);
		MakeTriangle(faces, leftBottom, indexCenter, rightBottom, indexBottom);
	} else if (sum == 1) {
		if (leftGap) {
			faces.emplace_back(Int3(leftTop, rightTop, indexLeft));
			faces.emplace_back(Int3(rightTop, rightBottom, indexLeft));
			faces.emplace_back(Int3(rightBottom, leftBottom, indexLeft));
		} else if (topGap) {
			faces.emplace_back(Int3(leftBottom, leftTop, indexTop));
			faces.emplace_back(Int3(rightBottom, leftBottom, indexTop));
			faces.emplace_back(Int3(rightTop, rightBottom, indexTop));
		} else if (rightGap) {
			faces.emplace_back(Int3(leftTop, rightTop, indexRight));
			faces.emplace_back(Int3(leftBottom, leftTop, indexRight));
			faces.emplace_back(Int3(rightBottom, leftBottom, indexRight));
		} else {
			assert(bottomGap);
			faces.emplace_back(Int3(leftBottom, leftTop, indexBottom));
			faces.emplace_back(Int3(leftTop, rightTop, indexBottom));
			faces.emplace_back(Int3(rightTop, rightBottom, indexBottom));
		}
	} else {
		assert(sum == 0);
		if (a > b) {
			faces.emplace_back(Int3(leftTop, rightTop, leftBottom));
			faces.emplace_back(Int3(rightBottom, leftBottom, rightTop));
		} else {
			faces.emplace_back(Int3(leftTop, rightBottom, leftBottom));
			faces.emplace_back(Int3(rightBottom, leftTop, rightTop));
		}
	}

	float diff = Math::Max(a, b);
	if (diff > deltaHeight) {
		// continue to divide
		indexCenter = indexCenter == -1 ? GenVertex(buffer, v) : indexCenter;

		uint32_t regionStart = (uint32_t)regions.size();
		for (size_t i = 0; i < 4; i++) {
			regions.emplace_back(Region());
		}

		const UInt2Pair& r = root.range;
		if (Detail || (r.first.x() == r.second.x() && r.first.y() == r.second.y())) {
			GenerateDetails<true>(regions[regionStart], regions, buffer, faces, UInt4(leftTop, indexTop, indexLeft, indexCenter), size, deltaHeight);
			GenerateDetails<true>(regions[regionStart + 1], regions, buffer, faces, UInt4(indexTop, rightTop, indexCenter, indexRight), size, deltaHeight);
			GenerateDetails<true>(regions[regionStart + 2], regions, buffer, faces, UInt4(indexLeft, indexCenter, leftBottom, indexBottom), size, deltaHeight);
			GenerateDetails<true>(regions[regionStart + 3], regions, buffer, faces, UInt4(indexCenter, indexRight, indexBottom, rightBottom), size, deltaHeight);
		} else {
			uint32_t k = 0;
			uint32_t x = ((r.second.x() - r.first.x() + 1) >> 1), y = ((r.second.y() - r.first.y() + 1) >> 1);
			for (uint32_t i = r.first.x(); i < r.second.x(); i += x) {
				for (uint32_t j = r.first.y(); j < r.second.y(); j += y) {
					uint32_t n = verify_cast<uint32_t>(regions.size()) - 4 + k;
					Region& subRegion = regions[n];
					UInt2Pair range = UInt2Pair(UInt2(i, j), UInt2(i + x, j + y));
					subRegion.range = range;
					GenerateDetails<false>(subRegion, regions, buffer, faces, UInt4(
						ToCacheIndex(range.first.x(), range.first.y(), totalWidth, totalHeight),
						ToCacheIndex(range.second.x(), range.first.y(), totalWidth, totalHeight),
						ToCacheIndex(range.first.x(), range.second.y(), totalWidth, totalHeight),
						ToCacheIndex(range.second.x(), range.second.y(), totalWidth, totalHeight)),
						size, deltaHeight);
					k++;
				}
			}
		}

		root.regionRange = std::make_pair(regionStart, (uint32_t)regions.size());
	}

	root.faceRange = std::make_pair(faceStart, (uint32_t)faces.size());
	root.box = Float3Pair(buffer[leftTop].vertex, buffer[rightBottom].vertex);
	Math::Union(root.box, buffer[rightTop].vertex);
	Math::Union(root.box, buffer[leftBottom].vertex);

	if (root.regionRange.first != 0) {
		for (uint32_t k = 0; k < 4; k++) {
			const Float3Pair& s = regions[root.regionRange.first + k].box;
			Math::Union(root.box, s.first);
			Math::Union(root.box, s.second);
		}
	}
}

static void ForCompile() {
	Region root;
	std::vector<Region> regions;
	std::vector<GradVertex> buffer;
	std::vector<Int3> faces;
	uint32_t leftTop = 0, rightTop = 0, leftBottom = 0, rightBottom = 0, totalWidth = 1, totalHeight = 1;
	float deltaHeight = 0.1f;
	UInt2 size(totalWidth, totalHeight);

	GenerateDetails<false>(root, regions, buffer, faces, UInt4(leftTop, rightTop, leftBottom, rightBottom), size, deltaHeight);
}