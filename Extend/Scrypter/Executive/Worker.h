// Worker.h
// PaintDream (paintdream@paintdream.com)
// 2022-9-27
//

#pragma once

namespace PaintsNow
{
	class Worker : public TReflected<Worker, CrossScript>
	{
	public:
		Worker(Kernel& kernel, IScript& script, uint32_t warp);
		~Worker() override;
		void ErrorHandler(IScript::Request& request, const String& err) override;
		void Dispatch(IScript::RequestPool* fromPool, IScript::RequestPool* toPool, ITask* task) override;

		Kernel& kernel;
		IScript::Request::Ref compileRoutine;
		TWrapper<void, Worker&, IScript::Request&, const String&> errorHandler;
	};
}

