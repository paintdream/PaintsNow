// EnvCubeComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "EnvCubeComponent.h"
#include "../Renderable/RenderableComponentModule.h"

namespace PaintsNow {
	class Entity;
	class EnvCubeComponent;
	class EnvCubeComponentModule : public TReflected<EnvCubeComponentModule, TRenderableComponentModule<EnvCubeComponent> > {
	public:
		EnvCubeComponentModule(Engine& engine);
		~EnvCubeComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create EnvCubeComponent
		/// </summary>
		/// <returns> EnvCubeComponent object </returns>
		TShared<EnvCubeComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Set texture of EnvCubeComponent
		/// </summary>
		/// <param name="envCubeComponent"> the EnvCubeComponent </param>
		/// <param name="textureResource"> cubemap texture resource </param>
		void RequestSetTexture(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, IScript::Delegate<TextureResource> textureResource);

		/// <summary>
		/// Set effect range of EnvCubeComponent
		/// </summary>
		/// <param name="envCubeComponent"> the EnvCubeComponent </param>`
		/// <param name="range"> cube effect range </param>
		void RequestSetRange(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, Float3& range);

		/// <summary>
		/// Set strength of EnvCubeComponent
		/// </summary>
		/// <param name="envCubeComponent"> the EnvCubeComponent </param>`
		/// <param name="strength"> strength of light </param>
		void RequestSetStrength(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, float strength);
	};
}

