// RenderFlowComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "RenderFlowComponent.h"
#include "../Renderable/RenderPolicy.h"
#include "../../Module.h"

namespace PaintsNow {
	class RenderStage;
	class RenderFlowComponent;
	class RenderFlowComponentModule : public TReflected<RenderFlowComponentModule, ModuleImpl<RenderFlowComponent> > {
	public:
		RenderFlowComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual void RegisterNodeTemplate(String& key, const TWrapper<RenderStage*, const String&>& t);

		/// <summary>
		/// Create RenderFlowComponent 
		/// </summary>
		/// <returns> RenderFlowComponent object </returns>
		TShared<RenderFlowComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Create RenderStage
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent </param>
		/// <param name="name"> name </param>
		/// <param name="config"> config string </param>
		/// <returns> RenderStage object </returns>
		TShared<RenderStage> RequestNewRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& name, const String& config);

		/// <summary>
		/// Enumerate ports of RenderStage
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent </param>
		/// <param name="renderStage"> the RenderStage </param>
		/// <returns> A array of render port names with { string } </returns>
		void RequestEnumerateRenderStagePorts(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> renderStage);

		/// <summary>
		/// Connect RenderPort to another
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent </param>
		/// <param name="from"> from RenderStage</param>
		/// <param name="fromPortName"> from port </param>
		/// <param name="to"> to RenderStage </param>
		/// <param name="toPortName"> to RenderStage port </param>
		void RequestLinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName);

		/// <summary>
		/// Disconnect RenderPort to another
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent </param>
		/// <param name="from"> from RenderStage</param>
		/// <param name="fromPortName"> from port </param>
		/// <param name="to"> to RenderStage </param>
		/// <param name="toPortName"> to RenderStage port </param>
		void RequestUnlinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName);

		/// <summary>
		/// Export port of RenderStage to public
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent </param>
		/// <param name="stage"> the RenderStage </param>
		/// <param name="portName"> the exported port name </param>
		/// <param name="symbol"> public symbol name </param>
		void RequestExportRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> stage, const String& portName, const String& symbol);

		/// <summary>
		/// Set material overrider for render stage
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent </param>
		/// <param name="renderStage"> the renderstage </param>
		/// <param name="renderTargetResource"> override materialResource </param>
		void RequestOverrideRenderStageMaterial(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> renderStage, IScript::Delegate<MaterialResource> materialResource);

		/// <summary>
		/// Bind output symbol with RenderTarget
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent </param>
		/// <param name="symbol"> the symbol name </param>
		/// <param name="renderTargetResource"> bound TextureResource </param>
		void RequestBindRenderTargetTexture(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& symbol, IScript::Delegate<TextureResource> renderTargetResource);

		/// <summary>
		/// Delete RenderStage
		/// </summary>
		/// <param name="renderFlow"> the RenderFlowComponent </param>
		/// <param name="renderStage"> the RenderStage to delete </param>
		void RequestDeleteRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlow, IScript::Delegate<RenderStage> renderStage);

	protected:
		std::map<String, TWrapper<RenderStage*, const String&> > stageTemplates;
	};
}
