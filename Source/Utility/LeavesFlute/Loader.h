// Loader.h
// PaintDream (paintdream@paintdream.com)
// 2016-1-1
//

#pragma once
#include "Config.h"
#include "CmdLine.h"
#include "../../Utility/LeavesFlute/LeavesFlute.h"

namespace PaintsNow {
	class Loader {
	public:
		Loader();
		~Loader();
		void Run(const CmdLine& cmdLine);
		Config& GetConfig();
		LeavesFlute*& GetLeavesFluteReference();
		TWrapper<void, LeavesFlute&> consoleHandler;
		TWrapper<void, LeavesFlute&> setupHandler;

	private:
		void SetFactory(TWrapper<IDevice*>& factory, const String& key, const std::map<String, CmdLine::Option>& factoryMap);
		IThread::Thread* mainThread;
		IThread* thread;
		IFrame* frame;
		LeavesFlute* leavesFlute;
		Config config;
	};
}
