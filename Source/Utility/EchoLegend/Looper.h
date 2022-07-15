#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/INetwork.h"
#include "../../Core/System/Kernel.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	class Looper : public TReflected<Looper, WarpTiny> {
	public:
		Looper(BridgeSunset& bridgeSunset, INetwork& network);
		~Looper() override;
		virtual bool Activate() = 0;
		virtual void Deactivate() = 0;
		void AsyncActivate(IScript::Request& request);
		static String EventToString(INetwork::EVENT event);

	protected:
		bool ActivateRoutine(IThread::Thread* thread, size_t);

	protected:
		INetwork& network;
		BridgeSunset& bridgeSunset;
		IThread::Thread* thread;
	};
}

