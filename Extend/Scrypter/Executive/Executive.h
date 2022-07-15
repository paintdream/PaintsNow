// Executive.h
// PaintDream (paintdream@paintdream.com)
// 2022-03-12
//

#pragma once

namespace PaintsNow
{
	class ExecRoutine : public TReflected<ExecRoutine, SharedTiny>
	{
	public:
		std::vector<TShared<CrossRoutine> > crossRoutines;
	};

	class Executive : public TReflected<Executive, IScript::Library>, public ThreadPool
	{
	public:
		Executive(uint32_t threadCount, const String& rootDirectory);
		~Executive() override;
		TObject<IReflect>& operator () (IReflect&) override;

		enum STATUS
		{
			EXECUTIVE_RUNNING,
			EXECUTIVE_SUSPENDED,
			EXECUTIVE_RESETTING,
		};

		bool InvokeReset();
		bool InvokeSuspend();
		bool InvokeResume();

		STATUS GetStatus() const;
		Kernel& GetKernel();
		const std::vector<TShared<Worker> >& GetWorkers() const;
		Worker& GetCurrentWorker();
		uint32_t GetCurrentWarpIndex() const;
		IArchive& GetArchive();
		void SetLogger(const TWrapper<void, const String&>& proc);

	protected:
		void RoutineReset();
		void Setup(Worker& worker);
		void OnScriptError(Worker& worker, IScript::Request& request, const String& err);
		void OnCompletion(DWORD bytes, ULONG_PTR key, OVERLAPPED* overlapped);
		void OnCompleteFile(DWORD bytes, ULONG_PTR key, File* file);
		HANDLE GetIocpHandle() const;
		void Signal() override;
		void Wait(uint32_t delay) override;

	protected:
		TShared<ExecRoutine> RequestCompile(IScript::Request& request, IScript::Request::Ref func);
		bool RequestDispatch(IScript::Request& request, IScript::Request::Ref callback, IScript::Delegate<ExecRoutine> func, IScript::Request::Arguments& args);

		void RequestQueryFiles(IScript::Request& request, StringView path);
		TShared<File> RequestOpenFile(IScript::Request& request, StringView path, uint32_t bufferSize, bool write);
		void RequestCloseFile(IScript::Request& request, IScript::Delegate<File> file);
		void RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, uint64_t offset, uint32_t size, IScript::Request::Ref callback);
		void RequestWriteFile(IScript::Request& request, IScript::Delegate<File> file, uint64_t offset, uint32_t size, StringView data, IScript::Request::Ref callback);
		int64_t RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file);
		void RequestWriteOutput(IScript::Request& request, IScript::Delegate<Document> document, StringView path, StringView content);
		bool RequestUpdateStatus(IScript::Request& request, IScript::Delegate<Document> document, StringView status, StringView detail, float ratio);

	protected:
		ZArchiveDirent archive;
		Kernel kernel;
		std::atomic<size_t> status;
		HANDLE iocpHandle;

		std::vector<TShared<Worker> > workers;
		TObjectAllocator<File> fileAllocator;
		String bootstrapCode;
		TWrapper<void, const String&> logger;
	};
}