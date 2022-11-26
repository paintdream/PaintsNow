// Worker.h
// PaintDream (paintdream@paintdream.com)
// 2022-9-27
//

#pragma once

namespace PaintsNow
{
	class Document;
	class Worker final : public TReflected<Worker, CrossScript>
	{
	public:
		Worker(Kernel& kernel, IScript& script, uint32_t warp);
		~Worker() override;

		void Clear();
		void ErrorHandler(IScript::Request& request, const String& err) override;
		void Dispatch(IScript::RequestPool* fromPool, IScript::RequestPool* toPool, ITask* task) override;
		void Queue(const TShared<Document>& document, IScript::Request::Ref callback);

		Kernel& kernel;
		TShared<Document> currentDocument;
		CrossScriptModule crossScriptModule;
		IScript::Request::Ref compileRoutine;
		DWORD currentThreadId;
		DWORD errorCount;
	};
}

