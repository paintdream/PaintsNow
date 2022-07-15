// WidgetComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../Batch/BatchComponentModule.h"
#include "../Renderable/RenderableComponentModule.h"
#include "WidgetComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class WidgetComponent;
	class WidgetComponentModule : public TReflected<WidgetComponentModule, TRenderableComponentModule<WidgetComponent> > {
	public:
		WidgetComponentModule(Engine& engine);
		void Initialize() override;
		void Uninitialize() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create WidgetComponent
		/// </summary>
		/// <param name="batchComponent"> the BatchComponent of WidgetComponent </param>
		/// <param name="material"> default MaterialResource object </param>
		/// <param name="mainTexture"> default TextureResource object </param>
		/// <returns> WidgetComponent object </returns>
		TShared<WidgetComponent> RequestNew(IScript::Request& request, IScript::Delegate<BatchComponent> batchComponent, IScript::Delegate<MaterialResource> material, IScript::Delegate<TextureResource> mainTexture);

		/// <summary>
		/// Set main texture of WidgetComponent
		/// </summary>
		/// <param name="widgetComponent"> the WidgetComponent </param>
		/// <param name="texture"> main TextureResource </param>
		void RequestSetWidgetTexture(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, IScript::Delegate<TextureResource> texture);

		/// <summary>
		/// Set widget texture coord info of WidgetComponent
		/// </summary>
		/// <param name="widgetComponent"> the WidgetComponent</param>
		/// <param name="texCoordBegin"> texture coord begin (uv0 scalable, uv1 repeatable) </param>
		/// <param name="texCoordEnd"> texture coord end (uv0 scalable, uv1 repeatable) </param>
		void RequestSetWidgetCoord(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, const Float4& begin, const Float4& end);

		/// <summary>
		/// Enable/Disable widget texture repeat mode
		/// </summary>
		/// <param name="widgetComponent"> the WidgetComponent </param>
		/// <param name="repeatable"> texture uv address repeatable </param>
		void RequestSetWidgetRepeatMode(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, bool repeatable);

	protected:
		TShared<MeshResource> widgetMesh;
		TShared<MaterialResource> defaultWidgetMaterial;
		TShared<TextureResource> defaultWidgetTexture;
		BatchComponentModule* batchComponentModule;
	};
}
