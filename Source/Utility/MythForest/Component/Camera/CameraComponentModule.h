// CameraComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "CameraComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class RenderFlowComponent;
	class EventComponentModule;
	class CameraComponentModule : public TReflected<CameraComponentModule, ModuleImpl<CameraComponent> > {
	public:
		CameraComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		/// <summary>
		/// Create CameraComponent
		/// </summary>
		/// <param name="renderFlowComponent"> a RenderFlowComponent that camera renders with </param>
		/// <param name="cameraViewPortName"> the binding viewport name of the RenderFlowComponent </param>
		/// <returns> A CameraComponent object </returns>
		TShared<CameraComponent> RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& cameraViewPortName);

		/// <summary>
		/// Get the count of collected entities in last tick
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <returns> the count of collected entities in last tick </returns>
		uint32_t RequestGetCollectedEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Get the count of collected visible entities in last tick
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <returns> the count of collected visible entities in last tick </returns>
		uint32_t RequestGetCollectedVisibleEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Get the count of collected triangles in last tick
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <returns> the count of collected triangles in last tick </returns>
		uint32_t RequestGetCollectedTriangleCount(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Get the count of collected draw calls in last tick
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <returns> the count of collected draw calls in last tick </returns>
		uint32_t RequestGetCollectedDrawCallCount(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Bind root entity for CameraComponent. The CameraComponent will traverse space frojm it.
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <param name="entity"> the root entity </param>
		void RequestBindRootEntity(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Set perspective parameters of CameraComponent
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <param name="fov"> field of view </param>
		/// <param name="nearPlane"> near clip plane </param>
		/// <param name="farPlane"> far clip plane </param>
		/// <param name="aspect"> screen ratio (width / height) </param>
		void RequestSetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent, float fov, float nearPlane, float farPlane, float aspect);

		/// <summary>
		/// Get perspective parameters of CameraComponent
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <returns> A dict with { "Fov" : number, "Near" : number, "Far" : number, "Aspect" : number } </returns>
		void RequestGetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Set visible distance of CameraComponent
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <param name="distance"> visible distance </param>
		void RequestSetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent, float distance);

		/// <summary>
		/// Get visible distance of CameraComponent
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <returns> visible distance </returns>
		float RequestGetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Enable/Disable projection jitter of CameraComponent (for temporal post effects, etc.)
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <param name="jitter"> enable jitter or not </param>
		void RequestSetProjectionJitter(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent, bool jitter);

		/// <summary>
		/// Recapture camera infos (camera transforms, environment changes, etc. between logic frames. without running ticking logic)
		/// </summary>
		/// <param name="cameraComponent"> the CameraComponent </param>
		/// <param name="smoothTrack"> enable smooth track or not </param>
		void RequestRefresh(IScript::Request& request, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Enable/Disable agile rendering (i.e. still render part of scene even some objects may not be ready.
		/// </summary>
		/// <param name="request"></param>
		/// <param name="enableAgileRendering"></param>
		void RequestSetAgileRendering(IScript::Request& request, IScript::Delegate<CameraComponent> camera, bool enableAgileRendering);

	protected:
		BridgeComponentModule* bridgeComponentModule;
	};
}
