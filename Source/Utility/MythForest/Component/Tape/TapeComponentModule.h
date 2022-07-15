// TapeComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "TapeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class TapeComponent;
	class TapeComponentModule : public TReflected<TapeComponentModule, ModuleImpl<TapeComponent> > {
	public:
		TapeComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create TapeComponent
		/// </summary>
		/// <param name="streamHolder"> stream holder entity </param>
		/// <param name="cacheBytes"> cache bytes </param>
		/// <returns></returns>
		TShared<TapeComponent> RequestNew(IScript::Request& request, IScript::Delegate<SharedTiny> streamHolder, size_t cacheBytes);

		/// <summary>
		/// Read next info from TapeComponent 
		/// </summary>
		/// <param name="tapeComponent"> the TapeComponent</param>
		/// <returns> A list with first element = node seq, second element = content </returns>
		std::pair<int64_t, String> RequestRead(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent);

		/// <summary>
		/// Write info to TapeComponent 
		/// </summary>
		/// <param name="tapeComponent"> the TapeComponent </param>
		/// <param name="seq"> sequence index </param>
		/// <param name="data"> data </param>
		/// <returns></returns>
		bool RequestWrite(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq, const String& data);

		/// <summary>
		/// Seek to specified sequence index 
		/// </summary>
		/// <param name="tapeComponent"> the TapeComponent </param>
		/// <param name="seq"> required sequence index </param>
		/// <returns></returns>
		bool RequestSeek(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq);

		/// <summary>
		/// Flush all write operations on TapeComponent
		/// </summary>
		/// <param name="tapeComponent"> the TapeComponent </param>
		/// <param name="asyncCallback"> non-null for asynchronized way </param>
		/// <returns></returns>
		bool RequestFlush(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, IScript::Request::Ref asyncCallback);
	};
}

