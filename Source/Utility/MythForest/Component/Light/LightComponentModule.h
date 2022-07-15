// LightComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "LightComponent.h"
#include "../Renderable/RenderableComponentModule.h"

namespace PaintsNow {
	class LightComponent;
	class LightComponentModule : public TReflected<LightComponentModule, TRenderableComponentModule<LightComponent> > {
	public:
		LightComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create LightComponent
		/// </summary>
		/// <returns> LightComponent object </returns>
		TShared<LightComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Set light source directinal or not
		/// </summary>
		/// <param name="lightComponent"> the LightComponent </param>
		/// <param name="directional"> if it is directinal </param>
		void RequestSetLightDirectional(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, bool directional);

		/// <summary>
		/// Set light color
		/// </summary>
		/// <param name="lightComponent"> the LightComponent </param>
		/// <param name="color"> the Light Color </param>
		void RequestSetLightColor(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, const Float3& color);

		/// <summary>
		/// Set light attenuation 
		/// </summary>
		/// <param name="lightComponent"> the LightComponent </param>
		/// <param name="attenuation"> light attenuationi </param>
		void RequestSetLightAttenuation(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float attenuation);

		/// <summary>
		/// Set light effect range
		/// </summary>
		/// <param name="lightComponent"> the LightComponent </param>
		/// <param name="range"> light range </param>
		void RequestSetLightRange(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, const Float3& range);

		/// <summary>
		/// Bind LightComponent with ShadowComponent to build a shadowmap streaming 
		/// </summary>
		/// <param name="lightComponent"> the LightComponent </param>
		/// <param name="layer"> CSM layer </param>
		/// <param name="streamComponent"> the StreamComponent </param>
		/// <param name="resolution"> shadowmap resolution </param>
		/// <param name="gridSize"> streaming grid size </param>
		/// <param name="scale"> streaming scale </param>
		void RequestBindLightShadowStream(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, uint32_t layer, IScript::Delegate<StreamComponent> streamComponent, const UShort2& resolution, float gridSize, float scale);
		// void RequestSetLightSpotAngle(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float spotAngle);
	};
}
