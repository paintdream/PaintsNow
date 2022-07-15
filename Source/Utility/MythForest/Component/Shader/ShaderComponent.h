// ShaderComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../SnowyStream/Resource/Passes/CustomMaterialPass.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	class ShaderComponent : public TAllocatedTiny<ShaderComponent, Component> {
	public:
		ShaderComponent(const TShared<ShaderResource>& shader, const String& name);
		~ShaderComponent() override;

		void SetInput(Engine& engine, const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config);
		void SetCode(Engine& engine, const String& stage, const String& code, const std::vector<std::pair<String, String> >& config);
		void SetComplete(Engine& engine, IScript::Request::Ref callback);
		TShared<MaterialResource> ExportMaterial(Engine& engine, const TShared<MaterialResource>& materialTemplate);

	protected:
		void OnShaderCompiled(IRender::Resource* resource, IRender::Resource::ShaderDescription&, IRender::Resource::ShaderDescription::Stage, const String&, const String&);

	protected:
		TShared<ShaderResourceImpl<CustomMaterialPass> > customMaterialShader;
		String name;
	};
}
