// CrossScriptModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "CrossScript.h"

namespace PaintsNow {
	class Entity;
	class CrossScript;
	class CrossRoutine;
	class CrossScriptModule : public TReflected<CrossScriptModule, IScript::Library> {
	public:
		CrossScriptModule(ThreadPool& threadPool, IScript& script);
		~CrossScriptModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<CrossScript> New(bool transparentMode, bool disableSandboxMode);

		/// <summary>
		/// Create CrossScript
		/// </summary>
		/// <param name="transparentMode"> if it is transparent mode (i.e. auto-convert remote functions to local function proxies) </param>
		/// <returns></returns>
		TShared<CrossScript> RequestNew(IScript::Request& request, bool transparentMode, bool disableSandBoxMode);

		/// <summary>
		/// Get CrossRoutine of local engine from global name.
		/// </summary>
		/// <param name="crossComponent"> the CrossScript </param>
		/// <param name="name"> name </param>
		/// <returns> CrossRoutine object </returns>
		TShared<CrossRoutine> RequestGet(IScript::Request& request, IScript::Delegate<CrossScript> crossComponent, const String& name);

		/// <summary>
		/// Load code on a CrossScript and get CrossRoutine of local engine.
		/// </summary>
		/// <param name="crossComponent"> the CrossScript </param>
		/// <param name="code"> code </param>
		/// <returns> CrossRoutine object </returns>
		TShared<CrossRoutine> RequestLoad(IScript::Request& request, IScript::Delegate<CrossScript> crossComponent, const String& code);

		/// <summary>
		/// Call CrossRoutine and wait it finishes.
		/// </summary>
		/// <param name="crossComponent"> the CrossScript </param>
		/// <param name="remoteRoutine"> the CrossRoutine </param>
		/// <param name="args"> arguments </param>
		/// <returns> all return values from remote call </returns>
		void RequestCall(IScript::Request& request, IScript::Delegate<CrossScript> crossComponent, IScript::Delegate<CrossRoutine> remoteRoutine, IScript::Request::Arguments& args);

		/// <summary>
		/// Call CrossRoutine in asynchronized way (blocked if waiting for target vm available
		/// </summary>
		/// <param name="crossComponent"> the CrossScript </param>
		/// <param name="callback"> callback on remote call finished </param>
		/// <param name="remoteRoutine"> the CrossRoutine </param>
		/// <param name="args"> arguments </param>
		void RequestCallAsync(IScript::Request& request, IScript::Delegate<CrossScript> crossComponent, IScript::Request::Ref callback, IScript::Delegate<CrossRoutine> remoteRoutine, IScript::Request::Arguments& args);

		/// <summary>
		/// Try to call CrossRoutine in asynchronized way, returns false if target is not available
		/// </summary>
		/// <param name="crossComponent"> the CrossScript </param>
		/// <param name="callback"> callback on remote call finished </param>
		/// <param name="remoteRoutine"> the CrossRoutine </param>
		/// <param name="args"> arguments </param>
		/// <returns> true if success </returns>
		bool RequestTryCallAsync(IScript::Request& request, IScript::Delegate<CrossScript> crossComponent, IScript::Request::Ref callback, IScript::Delegate<CrossRoutine> remoteRoutine, IScript::Request::Arguments& args);

		/// <summary>
		/// Test if target remote component is locked.
		/// </summary>
		/// <param name="crossComponent"> the CrossScript </param>
		/// <returns> whether locked </returns>
		bool RequestIsLocked(IScript::Request& request, IScript::Delegate<CrossScript> crossComponent);

		/// <summary>
		/// Cleanup CrossScript manually
		/// </summary>
		/// <param name="crossComponent"> the CrossScript </param>
		void RequestCleanup(IScript::Request& request, IScript::Delegate<CrossScript> crossComponent);

	protected:
		ThreadPool& threadPool;
		IScript& script;
	};
}

