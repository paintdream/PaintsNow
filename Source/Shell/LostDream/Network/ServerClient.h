// Allocator.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-27
//

#pragma once
#include "../LostDream.h"
#include "../../../General/Interface/INetwork.h"

namespace PaintsNow {
	class ServerClient : public TReflected<ServerClient, LostDream::Qualifier> {
	public:
		bool Initialize() override;
		bool Run(int randomSeed, int length) override;
		void Summary() override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		bool ThreadProc(IThread::Thread* thread, size_t id);
		void ClientEventHandler(ITunnel::Connection*, ITunnel::EVENT);
		void ServerEventHandler(ITunnel::Connection*, ITunnel::EVENT);
		const TWrapper<void, ITunnel::Connection*, ITunnel::EVENT> ConnectionHandler(ITunnel::Connection* connection);
		void EventHandler(ITunnel::Listener* listener, ITunnel::EVENT);

		INetwork* networkPtr;
		IThread* threadApiPtr;
	};
}