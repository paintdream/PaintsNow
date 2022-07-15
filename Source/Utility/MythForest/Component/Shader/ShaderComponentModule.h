// ShaderComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "ShaderComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class ShaderComponent;
	class ShaderComponentModule : public TReflected<ShaderComponentModule, ModuleImpl<ShaderComponent> > {
	public:
		ShaderComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create ShaderComponent
		/// </summary>
		/// <param name="shaderResource"> ShaderResource object </param>
		/// <param name="name"> name </param>
		/// <returns> ShaderComponent object </returns>
		TShared<ShaderComponent> RequestNew(IScript::Request& request, IScript::Delegate<ShaderResource> shaderResource, const String& name);

		/// <summary>
		/// Set shader code of ShaderComponent
		/// </summary>
		/// <param name="shaderComponent"> the ShaderComponent </param>
		/// <param name="stage"> shader stage string </param>
		/// <param name="text"> shader code text </param>
		/// <param name="config"> shader config </param>
		void RequestSetCode(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& text, const std::vector<std::pair<String, String> >& config);

		/// <summary>
		/// Set shader input config for stage of ShaderComponent
		/// </summary>
		/// <param name="shaderComponent"> the ShaderComponent </param>
		/// <param name="stage"> shader stage string </param>
		/// <param name="type"> input type </param>
		/// <param name="name"> input name </param>
		/// <param name="value"> input value </param>
		/// <param name="binding"> binding semantic </param>
		/// <param name="config"> config </param>
		void RequestSetInput(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config);

		/// <summary>
		/// Notify shader setting complete, start compiling
		/// </summary>
		/// <param name="shaderComponent"> the ShaderComponent </param>
		void RequestSetComplete(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, IScript::Request::Ref callback);

		/// <summary>
		/// Export a new material with shader in ShaderComponent
		/// </summary>
		/// <param name="shaderComponent"> the ShaderComponent </param>
		/// <param name="materialResource"> reference material, can be null </param>
		/// <returns> the new material </returns>
		TShared<MaterialResource> RequestExportMaterial(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, IScript::Delegate<MaterialResource> materialResource);
	};
}
