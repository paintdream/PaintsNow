// RemoteComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "RemoteComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class RemoteComponent;
	class RemoteRoutine;
	class RemoteComponentModule : public TReflected<RemoteComponentModule, ModuleImpl<RemoteComponent> > {
	public:
		RemoteComponentModule(Engine& engine);
		~RemoteComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create RemoteComponent
		/// </summary>
		/// <param name="transparentMode"> if it is transparent mode (i.e. auto-convert remote functions to local function proxies) </param>
		/// <param name="warpIndex"> warp index </param>
		/// <returns></returns>
		TShared<RemoteComponent> RequestNew(IScript::Request& request, bool transparentMode, uint32_t warpIndex);

		/// <summary>
		/// Get RemoteRoutine of local engine from global name.
		/// </summary>
		/// <param name="remoteComponent"> the RemoteComponent </param>
		/// <param name="name"> name </param>
		/// <returns> RemoteRoutine object </returns>
		TShared<RemoteRoutine> RequestGet(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, const String& name);

		/// <summary>
		/// Load code on a RemoteComponent and get RemoteRoutine of local engine.
		/// </summary>
		/// <param name="remoteComponent"> the RemoteComponent </param>
		/// <param name="code"> code </param>
		/// <returns> RemoteRoutine object </returns>
		TShared<RemoteRoutine> RequestLoad(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, const String& code);

		/// <summary>
		/// Call RemoteRoutine and wait it finishes.
		/// </summary>
		/// <param name="remoteComponent"> the RemoteComponent </param>
		/// <param name="remoteRoutine"> the RemoteRoutine </param>
		/// <param name="args"> arguments </param>
		/// <returns> all return values from remote call </returns>
		void RequestCall(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args);

		/// <summary>
		/// Call RemoteRoutine in asynchronized way (blocked if waiting for target vm available
		/// </summary>
		/// <param name="remoteComponent"> the RemoteComponent </param>
		/// <param name="callback"> callback on remote call finished </param>
		/// <param name="remoteRoutine"> the RemoteRoutine </param>
		/// <param name="args"> arguments </param>
		void RequestCallAsync(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, IScript::Request::Ref callback, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args);

		/// <summary>
		/// Try to call RemoteRoutine in asynchronized way, returns false if target is not available
		/// </summary>
		/// <param name="remoteComponent"> the RemoteComponent </param>
		/// <param name="callback"> callback on remote call finished </param>
		/// <param name="remoteRoutine"> the RemoteRoutine </param>
		/// <param name="args"> arguments </param>
		/// <returns> true if success </returns>
		bool RequestTryCallAsync(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, IScript::Request::Ref callback, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args);

		/// <summary>
		/// Test if target remote component is locked.
		/// </summary>
		/// <param name="remoteComponent"> the RemoteComponent </param>
		/// <returns> whether locked </returns>
		bool RequestIsLocked(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent);

		/// <summary>
		/// Cleanup RemoteComponent manually
		/// </summary>
		/// <param name="remoteComponent"> the RemoteComponent </param>
		void RequestCleanup(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent);
	};
}

