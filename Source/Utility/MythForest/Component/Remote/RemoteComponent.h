// RemoteComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	class RemoteRoutine : public TReflected<RemoteRoutine, SharedTiny> {
	public:
		RemoteRoutine(IScript::RequestPool* pool, IScript::Request::Ref ref);
		~RemoteRoutine() override;
		void ScriptUninitialize(IScript::Request& request) override;
		void Clear();

		IScript::RequestPool* pool;
		IScript::Request::Ref ref;
	};

	class RemoteComponent : public TAllocatedTiny<RemoteComponent, Component>, public IScript::RequestPool {
	public:
		enum {
			REMOTECOMPONENT_TRANSPARENT = COMPONENT_CUSTOM_BEGIN,
			REMOTECOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		RemoteComponent(Engine& engine);
		~RemoteComponent() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<RemoteRoutine> Load(const String& code);
		TShared<RemoteRoutine> Get(const String& name);
		void Call(IScript::Request& fromRequest, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args);
		void CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args);
		bool TryCallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args);

	protected:
		void FinishCallAsync(IScript::Request& fromRequest, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args);
		void Complete(IScript::RequestPool* returnPool, IScript::Request& request, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine);

	protected:
		Engine& engine;
	};
}

