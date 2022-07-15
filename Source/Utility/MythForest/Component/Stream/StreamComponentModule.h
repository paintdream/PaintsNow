// StreamComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "StreamComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class StreamComponent;
	class StreamComponentModule : public TReflected<StreamComponentModule, ModuleImpl<StreamComponent> > {
	public:
		StreamComponentModule(Engine& engine);
		~StreamComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// static int main(int argc, char* argv[]);

	public:
		/// <summary>
		/// Create StreamComponent 
		/// </summary>
		/// <param name="dimension"> dimension </param>
		/// <param name="cacheCount"> cache count </param>
		/// <returns> StreamComponent object </returns>
		TShared<StreamComponent> RequestNew(IScript::Request& request, const UShort3& dimension, uint16_t cacheCount);

		/// <summary>
		/// Set stream load handler
		/// </summary>
		/// <param name="streamComponent"> the StreamComponent </param>
		/// <param name="callback"> load callback </param>
		void RequestSetLoadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent, IScript::Request::Ref callback);

		/// <summary>
		/// Set stream refresh handler
		/// </summary>
		/// <param name="streamComponent"> the StreamComponent </param>
		/// <param name="callback"> refresh callback </param>
		void RequestSetRefreshHandler(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent, IScript::Request::Ref callback);

		/// <summary>
		/// Set stream unload handler 
		/// </summary>
		/// <param name="streamComponent"> the StreamComponent </param>
		/// <param name="callback"> unload callback </param>
		void RequestSetUnloadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent, IScript::Request::Ref callback);
	};
}

