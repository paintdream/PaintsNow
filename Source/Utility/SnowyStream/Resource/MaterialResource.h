// MaterialResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "ShaderResource.h"
#include "TextureResource.h"
#include "MeshResource.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class MaterialResource : public TReflected<MaterialResource, RenderResourceBase> {
	public:
		MaterialResource(ResourceManager& manager, const String& uniqueID);

		size_t ReportDeviceMemoryUsage() const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		void ScriptModify(IScript::Request& request, const String& action, IScript::Request::Arguments args) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ShaderResource> Instantiate(const TShared<MeshResource>& mesh, IRender::Resource::DrawCallDescription& drawCallTemplate, std::vector<Bytes>& bufferData);
		TShared<MaterialResource> CloneWithOverrideShader(const TShared<ShaderResource>& override);
		void Export(IRender& render, IRender::Queue* queue, const TShared<ShaderResource>& original);
		void Clear();
		void Import(const TShared<ShaderResource>& original);
		void MergeParameters(std::vector<IAsset::Material::Variable>& variables);
		uint64_t ComputeHashValue() const;

		IAsset::Material materialParams;
		TShared<ShaderResource> originalShaderResource;
		std::vector<TShared<TextureResource> > textureResources;

	protected:
		void ApplyMaterial(const TShared<ShaderResource>& original);
	};
}

