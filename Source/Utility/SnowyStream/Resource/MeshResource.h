// MeshResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "../../../General/Misc/PassBase.h"

namespace PaintsNow {
	class MeshResource : public TReflected<MeshResource, RenderResourceBase> {
	public:
		MeshResource(ResourceManager& manager, const String& uniqueID);
		~MeshResource() override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Refresh(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		bool UnMap() override;
		size_t ReportDeviceMemoryUsage() const override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		const Float3Pair& GetBoundingBox() const;

	public:
		IAsset::MeshCollection meshCollection;
		Float3Pair boundingBox;
		uint32_t deviceMemoryUsage;
		uint32_t deviceElementSize;

		class BufferCollection : public TReflected<BufferCollection, IReflectObjectComplex> {
		public:
			BufferCollection();
			TObject<IReflect>& operator () (IReflect& reflect) override;
			void GetDescription(std::vector<PassBase::Parameter>& desc, std::vector<std::pair<uint32_t, uint32_t> >& offsets, PassBase::Updater& updater) const;
			void UpdateData(std::vector<IRender::Resource*>& data) const;

			IRender::Resource* indexBuffer;
			IRender::Resource* positionBuffer;
			IRender::Resource* normalTangentColorBuffer;
			IRender::Resource* boneIndexWeightBuffer;
			bool hasNormalBuffer;
			bool hasTangentBuffer;
			bool hasColorBuffer;
			bool hasIndexWeightBuffer;
			std::vector<IRender::Resource*> texCoordBuffers;
		} bufferCollection;
	};
}

