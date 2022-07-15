// ShapeComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../../Core/Template/TKdTree.h"

namespace PaintsNow {
	class PatchRayCaster;
	class ShapeComponent : public TAllocatedTiny<ShapeComponent, Component> {
	public:
		ShapeComponent();
		~ShapeComponent() override;
		void Update(Engine& engine, const TShared<MeshResource>& resource);
		float Raycast(RaycastTask& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio = 1) const override;
		enum {
			MAX_PATCH_COUNT = 16
		};

		const TShared<MeshResource>& GetMesh() const;

	protected:
		void RoutineUpdate(Engine& engine, const TShared<MeshResource>& resource);
		void Cleanup();
		void Complete(RaycastTask& task, Float3Pair& ray, const MatrixFloat4x4& transform, Unit* parent, float ratio, void* context) const;

		struct VertexStorage {
			TVector<TVector<float, 4>, 3> vertices[MAX_PATCH_COUNT / 4][3];
		};

		struct_aligned(16) Patch : public TKdTree<Float3Pair, VertexStorage> {
			Patch* GetRight() const { return static_cast<Patch*>(rightNode); }
			Patch* GetLeft() const { return static_cast<Patch*>(leftNode); }

			uint32_t indices[MAX_PATCH_COUNT];
		};

		class PatchRayCaster;
		class PatchRayCuller;
		static void MakeHeapInternal(std::vector<Patch>& target, Patch* begin, Patch* end, uint8_t index);
		static Patch* MakeBound(Patch& patch, const std::vector<Float3>& vertices, const std::vector<UInt3>& indices);
		static void CheckBounding(Patch* root, Float3Pair& box);

		TShared<MeshResource> meshResource;
		std::vector<Patch> patches;
		Float3Pair boundingBox;
	};
}

