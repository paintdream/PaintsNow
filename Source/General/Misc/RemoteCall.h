// RemoteCall.h
// PaintDream (paintdream@paintdream.com)
// 2016-7-15
//

#pragma once
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../General/Interface/ITunnel.h"

namespace PaintsNow {
	class RemoteCall {
	public:
		enum STATUS { CONNECTED = 0, CLOSED, ABORTED, TIMEOUT };
		RemoteCall(IThread& threadApi, ITunnel& tunnel, IFilterBase& filter, const String& entry, const TWrapper<void, bool, STATUS, const String&>& statusHandler);
		~RemoteCall();

		bool Run();
		void Reset();
		void Stop();

	protected:
		const ITunnel::Handler OnConnection(ITunnel::Connection* connection);
		bool ThreadProc(IThread::Thread* thread, size_t context);
		void HandleEvent(ITunnel::EVENT event);

	protected:
		IThread& threadApi;
		ITunnel& tunnel;
		IFilterBase& filter;

		String entry;
		TWrapper<void, bool, STATUS, const String&> statusHandler;
		ITunnel::Dispatcher* dispatcher;
		std::atomic<IThread::Thread*> dispThread;
	};
}
