// SkeletonResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class SkeletonResource : public TReflected<SkeletonResource, RenderResourceBase> {
	public:
		SkeletonResource(ResourceManager& manager, const String& uniqueID);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Download(IRender& device, void* deviceContext) override;
		void Upload(IRender& device, void* deviceContext) override;
		void Attach(IRender& device, void* deviceContext) override;
		void Detach(IRender& device, void* deviceContext) override;
		typedef IAsset::BoneAnimation BoneAnimation;
		typedef IAsset::BoneAnimation::Joint Joint;
		typedef IAsset::TransSequence TransSequence;
		typedef IAsset::RotSequence RotSequence;
		typedef IAsset::ScalingSequence ScalingSequence;

		float Snapshot(std::vector<MatrixFloat4x4>& transforms, uint32_t clipIndex, float time, bool repeat);
		const BoneAnimation& GetBoneAnimation() const;
		const std::vector<MatrixFloat4x4>& GetOffsetTransforms() const;

		void UpdateBoneMatrixBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer, const std::vector<MatrixFloat4x4>& data);
		void ClearBoneMatrixBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer);

	protected:
		void PrepareTransform(std::vector<MatrixFloat4x4>& transforms, size_t i);
		void PrepareOffsetTransform(size_t i);

	protected:
		BoneAnimation boneAnimation;
		std::vector<MatrixFloat4x4> offsetMatrices;
		std::vector<MatrixFloat4x4> offsetMatricesInv;
	};
}

