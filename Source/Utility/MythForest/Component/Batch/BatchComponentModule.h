// BatchComponentModuleModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "BatchComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class BatchComponentModule : public TReflected<BatchComponentModule, ModuleImpl<BatchComponent> > {
	public:
		BatchComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		TShared<BatchComponent> Create(IRender::Resource::BufferDescription::Usage usage);

	public:
		/// <summary>
		/// Create BatchComponent
		/// </summary>
		/// <param name="usage"> batch type </param>
		/// <returns> BatchComponent object </returns>
		TShared<BatchComponent> RequestNew(IScript::Request& request, const String& usage);

		/// <summary>
		/// Get Statistics of a BatchComponnet
		/// </summary>
		/// <param name="batchComponent"> the BatchComponent </param>
		void RequestGetCaptureStatistics(IScript::Request& request, IScript::Delegate<BatchComponent> batchComponent);
	};
}

