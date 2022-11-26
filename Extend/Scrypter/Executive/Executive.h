// Executive.h
// PaintDream (paintdream@paintdream.com)
// 2022-03-12
//

#pragma once

namespace PaintsNow
{
	class ExecRoutine final : public TReflected<ExecRoutine, SharedTiny>
	{
	public:
		void ScriptUninitialize(IScript::Request& request) override;

		std::vector<TShared<CrossRoutine> > crossRoutines;
	};

	class Executive final : public TReflected<Executive, IScript::Library>, public ThreadPool
	{
	public:
		Executive(uint32_t threadCount, const String& rootDirectory, const String& bootstrapCode);
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
		void Clear();

	protected:
		void RoutineReset();
		void Setup(Worker& worker);
		void OnCompletion(DWORD bytes, ULONG_PTR key, OVERLAPPED* overlapped);
		void OnCompleteFile(DWORD bytes, ULONG_PTR key, File* file);
		HANDLE GetIocpHandle() const;
		void Signal() override;
		void Wait(uint32_t delay) override;

	protected:
		TShared<ExecRoutine> RequestCompile(IScript::Request& request, IScript::Request::Ref func);
		bool RequestDispatch(IScript::Request& request, IScript::Request::Ref callback, IScript::Delegate<ExecRoutine> func, IScript::Request::Arguments& args);
		String RequestCompileRemote(IScript::Request& request, IScript::Request::Ref func);
		bool RequestDispatchRemote(IScript::Request& request, IScript::Request::Ref callback, StringView func, StringView args, const std::vector<StringView>& syncFiles);
		void RequestQueue(IScript::Request& request, IScript::Request::Ref callback);
		void RequestSleep(IScript::Request& request, uint64_t milliseconds);

		void RequestShellExecute(IScript::Request& request, StringView operation, StringView program, StringView params, bool showWindow);
		void RequestQueryFiles(IScript::Request& request, StringView path, StringView matcher);
		TShared<File> RequestOpenFile(IScript::Request& request, StringView path, uint32_t bufferSize, bool write);
		void RequestCloseFile(IScript::Request& request, IScript::Delegate<File> file);
		void RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, uint64_t offset, uint32_t size, IScript::Request::Ref callback);
		void RequestWriteFile(IScript::Request& request, IScript::Delegate<File> file, uint64_t offset, uint32_t size, StringView data, IScript::Request::Ref callback);
		int64_t RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file);
		void RequestRecord(IScript::Request& request, StringView object, StringView description, StringView cookie);
		void RequestLog(IScript::Request& request, const String& logText);
		void RequestUpdateProfile(IScript::Request& request, StringView status, StringView detail, float ratio);
		size_t RequestLoadLibrary(IScript::Request& request, const String& library);
		size_t RequestGetSymbolAddress(IScript::Request& request, size_t handle, const String& entry);
		size_t RequestCallSymbol(IScript::Request& request, bool isStdCall, size_t address, IScript::Request::Arguments& args);
		bool RequestFreeLibrary(IScript::Request& request, size_t handle);

	protected:
		ZArchiveDirent archive;
		Kernel kernel;
		std::atomic<size_t> status;
		HANDLE iocpHandle;

		std::vector<TShared<Worker> > workers;
		TObjectAllocator<File> fileAllocator;
		String rootDirectory;
		String bootstrapCode;
	};
}