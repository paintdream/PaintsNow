#include "stdafx.h"
using namespace PaintsNow;

enum 
{
	COMPLETION_FILE_READ = 1,
	COMPLETION_FILE_WRITE,
};

void ExecRoutine::ScriptUninitialize(IScript::Request& request) {
	request.UnLock();
	BaseClass::ScriptUninitialize(request);
	request.DoLock();
}

static ZThreadPthread GThreadApi;
Executive::Executive(uint32_t threadCount, const String& rd, const String& bc) : ThreadPool(GThreadApi, threadCount), kernel(*this, GetThreadCount() + 1), archive(threadApi, rd), rootDirectory(rd), bootstrapCode(bc)
{
	/*
	static ZNetworkLibEvent GNetworkLibEvent(GThreadApi);
	static ZFilterPod GFilterPod;
	Coordinator coordinator(GThreadApi, GNetworkLibEvent, GFilterPod, "");*/

	status.store(EXECUTIVE_RUNNING, std::memory_order_relaxed);

	uint32_t warpCount = kernel.GetWarpCount(); // 1:1 model, plus an extra worker for scheduling
	for (uint32_t i = 0; i < warpCount; i++)
	{
		IScript* script = new ZScriptLua(threadApi);
		Worker* worker = new Worker(kernel, *script, i);
		Setup(*worker);
		workers.emplace_back(TShared<Worker>::From(worker));
	}
	
	iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, (ULONG_PTR)this, threadCount);
}

Executive::~Executive()
{
	Clear();
	::CloseHandle(iocpHandle);
}

void Executive::Clear()
{
	for (size_t i = 0; i < workers.size(); i++)
	{
		Worker& worker = *workers[i];
		worker.Clear();
	}

	workers.clear();
}

HANDLE Executive::GetIocpHandle() const
{
	return iocpHandle;
}

void Executive::Signal()
{
	::PostQueuedCompletionStatus(iocpHandle, 0, 0, nullptr);
}

void Executive::Wait(uint32_t delay)
{
	DWORD interval = delay == ~(uint32_t)0 ? INFINITE : delay;
	DWORD bytes = 0;
	ULONG_PTR key = 0;
	LPOVERLAPPED overlapped = nullptr;

	std::atomic<size_t>& counter = reinterpret_cast<std::atomic<size_t>&>(waitEventCounter);
	counter.fetch_add(1, std::memory_order_acquire);

	while (::GetQueuedCompletionStatus(iocpHandle, &bytes, &key, &overlapped, interval))
	{
		// valid file operation?
		if (overlapped != nullptr)
		{
			OnCompletion(bytes, key, overlapped);
		}
		else
		{
			break;
		}
	}

	counter.fetch_sub(1, std::memory_order_release);
}

Executive::STATUS Executive::GetStatus() const
{
	return verify_cast<STATUS>(status.load(std::memory_order_acquire));
}

Kernel& Executive::GetKernel()
{
	return kernel;
}

IArchive& Executive::GetArchive()
{
	return archive;
}

bool Executive::InvokeReset()
{
	size_t expected = EXECUTIVE_RUNNING;
	if (status.compare_exchange_strong(expected, EXECUTIVE_RESETTING, std::memory_order_release))
	{
		Dispatch(CreateTaskContextFree(Wrap(this, &Executive::RoutineReset)));
		return true;
	}
	else
	{
		return false;
	}
}

void Executive::RoutineReset()
{
	assert(status.load(std::memory_order_acquire) == EXECUTIVE_RESETTING);
	kernel.Reset();
	for (size_t i = 0; i < workers.size(); i++)
	{
		Worker& worker = *workers[i];
		worker.GetScript().Reset();
		Setup(worker);
	}

	assert(status.load(std::memory_order_acquire) == EXECUTIVE_RESETTING);
	status.store(EXECUTIVE_RUNNING, std::memory_order_release);
}

bool Executive::InvokeSuspend()
{
	size_t expected = EXECUTIVE_RUNNING;
	if (status.compare_exchange_strong(expected, EXECUTIVE_RESETTING, std::memory_order_relaxed))
	{
		uint32_t warpCount = kernel.GetWarpCount();
		for (uint32_t i = 0; i < warpCount; i++)
		{
			kernel.SuspendWarp(i);
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool Executive::InvokeResume()
{
	size_t expected = EXECUTIVE_RESETTING;
	if (status.compare_exchange_strong(expected, EXECUTIVE_RUNNING, std::memory_order_relaxed))
	{
		uint32_t warpCount = kernel.GetWarpCount();
		for (uint32_t i = 0; i < warpCount; i++)
		{
			kernel.ResumeWarp(i);
		}

		return true;
	}
	else
	{
		return false;
	}
}

void Executive::OnCompleteFile(DWORD bytes, ULONG_PTR key, File* file)
{
	Document* document = file->GetDocument();
	TShared<Document> lastDocument = document->PushDocument();

	IScript::Request::Ref callback = file->GetCallback();
	Worker& worker = GetCurrentWorker();
	IScript::Request& request = *worker.allocate(1);
	request.DoLock();

	if (key == COMPLETION_FILE_READ)
	{
		request.Call(callback, file, StringView(file->GetBuffer(), bytes));
	}
	else
	{
		request.Call(callback, file);
	}

	request.UnLock();
	worker.deallocate(&request, 1);

	document->PopDocument(lastDocument);
	file->Release();
	file->ReleaseObject();
}

void Executive::OnCompletion(DWORD bytes, ULONG_PTR key, OVERLAPPED* overlapped)
{
	switch (key)
	{
		case COMPLETION_FILE_READ:
		case COMPLETION_FILE_WRITE:
		{
			File* file = File::FromOverlapped(overlapped);
			kernel.QueueRoutine(file, CreateTaskContextFree(Wrap(this, &Executive::OnCompleteFile), bytes, key, file));
			break;
		}
	}
}

void Executive::Setup(Worker& worker)
{
	IScript& script = worker.GetScript();
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();

	request << global
		<< key("Executive") << *this // register global functions
		<< key("CrossScriptModule") << worker.crossScriptModule
		<< key("RootDirectory") << rootDirectory
		<< key("ThreadCount") << GetThreadCount()
		<< endtable;

	IScript::Request::Ref ref = request.Load(bootstrapCode, "Core");
	request.Push();
	bool success = request.Call(ref);
	assert(success);
	request.Pop();

	assert(!worker.compileRoutine);
	request << global
		<< key("_CompileRoutine") >> worker.compileRoutine
		<< endtable;
	assert(worker.compileRoutine);

	request.Dereference(ref);
	request.UnLock();
}

const std::vector<TShared<Worker> >& Executive::GetWorkers() const
{
	return workers;
}

uint32_t Executive::GetCurrentWarpIndex() const
{
	return kernel.GetCurrentWarpIndex();
}

Worker& Executive::GetCurrentWorker()
{
	uint32_t warp = GetCurrentWarpIndex();
	assert(warp != ~(uint32_t)0);
	assert(warp == workers[warp]->GetWarpIndex());

	return *workers[warp];
}

TObject<IReflect>& Executive::operator () (IReflect& reflect)
{
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod())
	{
		ReflectMethod(RequestCompile)[ScriptMethodLocked = "Compile"];
		ReflectMethod(RequestDispatch)[ScriptMethodLocked = "Dispatch"];
		ReflectMethod(RequestCompileRemote)[ScriptMethodLocked = "CompileRemote"];
		ReflectMethod(RequestDispatchRemote)[ScriptMethodLocked = "DispatchRemote"];
		ReflectMethod(RequestQueue)[ScriptMethodLocked = "Queue"];
		ReflectMethod(RequestSleep)[ScriptMethod = "Sleep"];

		ReflectMethod(RequestShellExecute)[ScriptMethodLocked = "ShellExecute"];
		ReflectMethod(RequestQueryFiles)[ScriptMethodLocked = "QueryFiles"];
		ReflectMethod(RequestOpenFile)[ScriptMethodLocked = "OpenFile"];
		ReflectMethod(RequestCloseFile)[ScriptMethodLocked = "CloseFile"];
		ReflectMethod(RequestReadFile)[ScriptMethodLocked = "ReadFile"];
		ReflectMethod(RequestWriteFile)[ScriptMethodLocked = "WriteFile"];
		ReflectMethod(RequestGetFileSize)[ScriptMethodLocked = "GetFileSize"];
		ReflectMethod(RequestLog)[ScriptMethodLocked = "Log"];
		ReflectMethod(RequestRecord)[ScriptMethodLocked = "Record"];
		ReflectMethod(RequestUpdateProfile)[ScriptMethodLocked = "UpdateProfile"];
		ReflectMethod(RequestLoadLibrary)[ScriptMethod = "LoadLibrary"];
		ReflectMethod(RequestGetSymbolAddress)[ScriptMethod = "GetSymbolAddress"];
		ReflectMethod(RequestCallSymbol)[ScriptMethod = "CallSymbol"];
		ReflectMethod(RequestFreeLibrary)[ScriptMethod = "FreeLibrary"];
	}

	return *this;
}

TShared<ExecRoutine> Executive::RequestCompile(IScript::Request& request, IScript::Request::Ref func)
{
	CHECK_REFERENCES_WITH_TYPE_LOCKED(func, IScript::Request::FUNCTION);

	uint32_t warp = kernel.GetCurrentWarpIndex();
	Worker& worker = GetCurrentWorker();
	std::vector<IScript::Delegate<CrossRoutine> > routines;
	request.Push();
	request.Call(worker.compileRoutine, workers, func);
	request >> routines;
	request.Dereference(func);
	request.Pop();

	TShared<ExecRoutine> execRoutine = TShared<ExecRoutine>::From(new ExecRoutine());
	execRoutine->crossRoutines.reserve(routines.size());
	for (size_t i = 0; i < routines.size(); i++)
	{
		execRoutine->crossRoutines.emplace_back(routines[i].Get());
	}

	return execRoutine;
}

bool Executive::RequestDispatch(IScript::Request& request, IScript::Request::Ref callback, IScript::Delegate<ExecRoutine> func, IScript::Request::Arguments& args)
{
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(func);

	Worker& currentWorker = GetCurrentWorker();
	uint32_t targetWarp = kernel.PushIdleWarp();
	if (targetWarp != ~(uint32_t)0)
	{
		Worker& instance = *workers[targetWarp];
		assert(&instance != &currentWorker);

		const TShared<CrossRoutine>& routine = func.Get()->crossRoutines[targetWarp];
		assert(routine);

		TShared<Document> stash = instance.currentDocument;
		instance.currentDocument = currentWorker.currentDocument;

		request.UnLock();
		instance.CallAsync(request, callback, routine, args);
		request.DoLock();

		instance.currentDocument = stash;
		kernel.PopWarp();
		return true;
	}

	request.Dereference(callback);
	// all script engine busy, or target name not found
	return false;
}

void Executive::RequestShellExecute(IScript::Request& request, StringView operation, StringView program, StringView params, bool showWindow)
{
	::ShellExecuteW(nullptr, (LPCWSTR)Utf8ToSystem(operation).c_str(), (LPCWSTR)Utf8ToSystem(program).c_str(), (LPCWSTR)Utf8ToSystem(params).c_str(), nullptr, showWindow ? SW_SHOW : SW_HIDE);
}

void Executive::RequestQueue(IScript::Request& request, IScript::Request::Ref callback)
{
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);

	Worker& worker = GetCurrentWorker();
	kernel.QueueRoutinePost(&worker, CreateTaskContextFree(Wrap(&worker, &Worker::Queue), worker.currentDocument, callback));
}

void Executive::RequestSleep(IScript::Request& request, uint64_t milliseconds)
{
	::Sleep((DWORD)milliseconds);
}

void Executive::RequestQueryFiles(IScript::Request& request, StringView path, StringView matcher)
{
	String filter = Utf8ToSystem(matcher);
	const WCHAR* strFilter = reinterpret_cast<const WCHAR*>(filter.c_str());

	WIN32_FIND_DATAW findData;
	request << beginarray;
	HANDLE findHandle = ::FindFirstFileW((LPCWSTR)Utf8ToSystem(String(path) + "*").c_str(), &findData);
	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (findData.cFileName[0] == _T('.'))
				continue;

			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !::PathMatchSpecW(findData.cFileName, strFilter))
				continue;

			String file = SystemToUtf8(String((const char*)findData.cFileName, wcslen(findData.cFileName) * 2));
			if (!!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				file += '/';
			}

			request << file;
		}
		while (::FindNextFileW(findHandle, &findData));

		::FindClose(findHandle);
	}

	request << endarray;
}

TShared<File> Executive::RequestOpenFile(IScript::Request& request, StringView path, uint32_t bufferSize, bool write)
{
	HANDLE fileHandle = ::CreateFileW((LPCWSTR)Utf8ToSystem(path).c_str(), write ? FILE_GENERIC_WRITE | FILE_GENERIC_READ : FILE_GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		File* file = fileAllocator.New(GetCurrentWorker().currentDocument(), fileHandle, GetIocpHandle(), bufferSize);
		file->SetWarpIndex(kernel.GetCurrentWarpIndex());
		return TShared<File>::From(file);
	}
	else
	{
		return nullptr;
	}
}

void Executive::RequestCloseFile(IScript::Request& request, IScript::Delegate<File> file)
{
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	file->Close();
}

void Executive::RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, uint64_t offset, uint32_t size, IScript::Request::Ref callback)
{
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(file);

	if (size > file->GetBufferSize())
	{
		request.Error("Size is larger than buffer size!");
		return;
	}

	if (file->Acquire())
	{
		HANDLE handle = file->GetHandle();
		assert(handle != INVALID_HANDLE_VALUE);
		OVERLAPPED* overlapped = file->GetOverlapped();
		overlapped->Offset = (DWORD)(offset & 0xffffffff);
		overlapped->OffsetHigh = (DWORD)((offset >> 31) >> 1);
		// check whether is it using by another session

		file->ReferenceObject();
		file->SetCallback(callback);
		file->SetWarpIndex(kernel.GetCurrentWarpIndex());

		if (::ReadFile(handle, file->GetBuffer(), (DWORD)size, nullptr, overlapped))
		{
			return;
		}
		else
		{
			file->Release();
			file->ReleaseObject();
			request.Error("Unable to schedule reading a file.");
		}
	}
	else
	{
		request.Error("Unable to read a pending file");
	}

	request.Dereference(callback);
}

void Executive::RequestWriteFile(IScript::Request& request, IScript::Delegate<File> file, uint64_t offset, uint32_t size, StringView data, IScript::Request::Ref callback)
{
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(file);

	if (size > file->GetBufferSize())
	{
		request.Error("Size is larger than buffer size!");
		return;
	}

	if (file->Acquire())
	{
		HANDLE handle = file->GetHandle();
		assert(handle != INVALID_HANDLE_VALUE);
		OVERLAPPED* overlapped = file->GetOverlapped();
		overlapped->Offset = (DWORD)(offset & 0xffffffff);
		overlapped->OffsetHigh = (DWORD)((offset >> 31) >> 1);
		// check whether is it using by another session
		memcpy(file->GetBuffer(), data.data(), size);

		file->ReferenceObject();
		file->SetCallback(callback);
		file->SetWarpIndex(kernel.GetCurrentWarpIndex());

		if (::WriteFile(handle, file->GetBuffer(), (DWORD)size, nullptr, overlapped))
		{
			return;
		}
		else
		{
			file->Release();
			file->ReleaseObject();
			request.Error("Unable to schedule writing a file.");
		}
	}
	else
	{
		request.Error("Unable to write a pending file");
	}

	request.Dereference(callback);
}

int64_t Executive::RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file)
{
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	
	LARGE_INTEGER size;
	if (::GetFileSizeEx(file->GetHandle(), &size))
	{
		return size.QuadPart;
	}
	else
	{
		return -1;
	}
}

void Executive::RequestRecord(IScript::Request& request, StringView object, StringView description, StringView cookie)
{
	CHECK_REFERENCES_NONE();
	GetCurrentWorker().currentDocument->WriteRecord(object, description, cookie);
}

void Executive::RequestLog(IScript::Request& request, const String& logText)
{
	GetCurrentWorker().currentDocument->WriteLog(logText);
}

void Executive::RequestUpdateProfile(IScript::Request& request, StringView status, StringView detail, float ratio)
{
	CHECK_REFERENCES_NONE();
	GetCurrentWorker().currentDocument->UpdateProfile(status, detail, ratio);
}

size_t Executive::RequestLoadLibrary(IScript::Request& request, const String& path)
{
	WCHAR fullPathName[MAX_PATH * 4];
	::GetFullPathNameW((const WCHAR*)Utf8ToSystem(path).c_str(), MAX_PATH * 4, fullPathName, nullptr);
	return (size_t)::LoadLibraryW(fullPathName);
}

size_t Executive::RequestGetSymbolAddress(IScript::Request& request, size_t handle, const String& entry)
{
	return (size_t)(void*)::GetProcAddress((HMODULE)handle, entry.c_str());
}

static const size_t MAX_PARAM_COUNT = 8;
static size_t CallAddress(bool isStdCall, size_t address, size_t params[], size_t count, const char*& message)
{
	size_t ret = 0;
	__try
	{
		if (isStdCall)
		{
			switch (count)
			{
			case 0:
				ret = ((size_t(_stdcall*)())address)();
				break;
			case 1:
				ret = ((size_t(_stdcall*)(size_t))address)(params[0]);
				break;
			case 2:
				ret = ((size_t(_stdcall*)(size_t, size_t))address)(params[0], params[1]);
				break;
			case 3:
				ret = ((size_t(_stdcall*)(size_t, size_t, size_t))address)(params[0], params[1], params[2]);
				break;
			case 4:
				ret = ((size_t(_stdcall*)(size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3]);
				break;
			case 5:
				ret = ((size_t(_stdcall*)(size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4]);
				break;
			case 6:
				ret = ((size_t(_stdcall*)(size_t, size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4], params[5]);
				break;
			case 7:
				ret = ((size_t(_stdcall*)(size_t, size_t, size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
				break;
			}
		}
		else
		{
			switch (count)
			{
			case 0:
				ret = ((size_t(*)())address)();
				break;
			case 1:
				ret = ((size_t(*)(size_t))address)(params[0]);
				break;
			case 2:
				ret = ((size_t(*)(size_t, size_t))address)(params[0], params[1]);
				break;
			case 3:
				ret = ((size_t(*)(size_t, size_t, size_t))address)(params[0], params[1], params[2]);
				break;
			case 4:
				ret = ((size_t(*)(size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3]);
				break;
			case 5:
				ret = ((size_t(*)(size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4]);
				break;
			case 6:
				ret = ((size_t(*)(size_t, size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4], params[5]);
				break;
			case 7:
				ret = ((size_t(*)(size_t, size_t, size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
				break;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		message = "Native code exception captured!";
	}

	return ret;
}

size_t Executive::RequestCallSymbol(IScript::Request& request, bool isStdCall, size_t addressValue, IScript::Request::Arguments& args)
{
	size_t ret = 0;
	if (addressValue != 0)
	{
		void* address = reinterpret_cast<void*>(addressValue);
		size_t params[MAX_PARAM_COUNT] = { 0 };
		for (size_t i = 0; i < args.count; i++)
		{
			if (request.GetCurrentType() == IScript::Request::STRING)
			{
				request >> reinterpret_cast<const char*&>(params[i]);
			}
			else
			{
				request >> params[i];
			}
		}

		request.UnLock();

		const char* message = nullptr;
		CallAddress(isStdCall, addressValue, params, args.count, message);

		request.DoLock();
		if (message != nullptr)
			request.Error(message);
	}

	return ret;
}

bool Executive::RequestFreeLibrary(IScript::Request& request, size_t handle)
{
	return ::FreeLibrary((HMODULE)handle) != 0;
}

String Executive::RequestCompileRemote(IScript::Request& request, IScript::Request::Ref func)
{
	// TODO:
	return String();
}

bool Executive::RequestDispatchRemote(IScript::Request& request, IScript::Request::Ref callback, StringView func, StringView args, const std::vector<StringView>& syncFiles)
{
	// TODO:
	return false;
}

