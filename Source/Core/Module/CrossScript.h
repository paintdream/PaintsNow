// CrossScript.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../Interface/IScript.h"
#include "../System/Kernel.h"
#include "../Template/TAllocator.h"

namespace PaintsNow {
	class CrossRoutine : public TReflected<CrossRoutine, SharedTiny> {
	public:
		CrossRoutine(IScript::RequestPool* pool, IScript::Request::Ref ref);
		~CrossRoutine() override;
		void ScriptUninitialize(IScript::Request& request) override;
		void Clear();

		IScript::RequestPool* pool;
		IScript::Request::Ref ref;
	};

	class CrossScript : public TReflected<CrossScript, WarpTiny>, public IScript::RequestPool {
	public:
		enum {
			CROSSSCRIPT_TRANSPARENT = WARP_CUSTOM_BEGIN,
			CROSSSCRIPT_CUSTOM_BEGIN = WARP_CUSTOM_BEGIN << 1
		};

		// Note: CrossScript will take `script` ownership!
		CrossScript(ThreadPool& threadPool, IScript& script);
		~CrossScript() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<CrossRoutine> Load(const String& code);
		TShared<CrossRoutine> Get(const String& name);
		void Call(IScript::Request& fromRequest, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args);
		void CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args);
		bool TryCallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args);
		bool IsLocked() const;

	protected:
		void ScriptUninitialize(IScript::Request& request) override;
		virtual void PrepareCall(IScript::Request& fromRequest, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args);
		virtual void ExecuteCall(IScript::RequestPool* returnPool, IScript::Request& request, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine);
		virtual void CompleteCall(IScript::RequestPool* returnPool, IScript::Request& request, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine);
		virtual void ErrorHandler(IScript::Request& request, const String& err);
		virtual void Dispatch(IScript::RequestPool* fromPool, IScript::RequestPool* toPool, ITask* task);

	protected:
		ThreadPool& threadPool;
	};
}

