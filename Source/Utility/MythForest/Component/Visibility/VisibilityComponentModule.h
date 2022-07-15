// VisibilityComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "VisibilityComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class VisibilityComponent;
	class VisibilityComponentModule : public TReflected<VisibilityComponentModule, ModuleImpl<VisibilityComponent> > {
	public:
		VisibilityComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create VisibilityComponent
		/// </summary>
		/// <param name="streamComponent"> the StreamComponent for stream visibility cubes </param>
		/// <returns> VisibilityComponent object </returns>
		TShared<VisibilityComponent> RequestNew(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent);

		/// <summary>
		/// Setup parameters of VisibilityComponent
		/// </summary>
		/// <param name="visibilityComponent"> the VisibilityComponent </param>
		/// <param name="maxDistance"> max view distance </param>
		/// <param name="gridSize"> grid size </param>
		/// <param name="taskCount"> render task limit of single render frame </param>
		/// <param name="pixelThreshold"> pixel count threshold for killing objects </param>
		/// <param name="resolution"> visibility texture resolution </param>
		void RequestSetup(IScript::Request& request, IScript::Delegate<VisibilityComponent> visibilityComponent, float maxDistance, const Float3& gridSize, uint32_t taskCount, uint32_t pixelThreshold, const UShort2& resolution);
	};
}
