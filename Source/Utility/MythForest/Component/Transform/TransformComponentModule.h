// TranformComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "TransformComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class TransformComponent;
	class TransformComponentModule : public TReflected<TransformComponentModule, ModuleImpl<TransformComponent> > {
	public:
		TransformComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create TransformComponent
		/// </summary>
		/// <returns> TransformComponent object with identity transform </returns>
		TShared<TransformComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Make rotation on editor, usually for camera operations
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <param name="from"> from point (screen space) </param>
		/// <param name="to"> to point (screen space) </param>
		void RequestEditorRotate(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float2& from, Float2& to);

		/// <summary>
		/// Set rotation of TransformComponent
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <param name="rotation"></param>
		void RequestSetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& rotation);

		/// <summary>
		/// Get rotation of TransformComponent
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <returns> rotation in eular angles </returns>
		Float3 RequestGetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);

		/// <summary>
		/// Set scale of TransformComponent 
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <param name="scale"> scale </param>
		void RequestSetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& scale);

		/// <summary>
		/// Get scale of TransformComponent 
		/// </summary>
		/// <param name="request"></param>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <returns> scale </returns>
		Float3 RequestGetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);

		/// <summary>
		/// Set translation of TransformComponent
		/// </summary>
		/// <param name="request"></param>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <param name="translation"> translationi </param>
		void RequestSetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& translation);

		/// <summary>
		/// Get translation of TransformComponent 
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <returns> translation of TransformComponent</returns>
		Float3 RequestGetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);

		/// <summary>
		/// Get axises of TransformComponent
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <returns> A array with { Vector3 } of size 3, indicating 3 main axises of TransformComponent </returns>
		void RequestGetAxises(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);

		/// <summary>
		/// Get quick translation of TransformComponent
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <returns> translation </returns>
		Float3 RequestGetQuickTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);

		/// <summary>
		/// Set transform as dynamic or not
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <param name="isDynamic"> dynamic </param>
		/// <returns> translation </returns>
		void RequestSetDynamic(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, bool isDynamic);

		/// <summary>
		/// Get transform dynamic
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		/// <returns> dynamic </returns>
		bool RequestGetDynamic(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);

		/// <summary>
		/// Apply modifications on translation/scale/rotation to TransformComponent
		/// </summary>
		/// <param name="transformComponent"> the TransformComponent </param>
		void RequestUpdateTransform(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
	};
}

