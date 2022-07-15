#include "stdafx.h"
using namespace PaintsNow;

enum 
{
	COMPLETION_FILE_READ = 1,
	COMPLETION_FILE_WRITE,
};

static ZThreadPthread GThreadApi;
Executive::Executive(uint32_t threadCount, const String& rootDirectory) : ThreadPool(GThreadApi, threadCount), kernel(*this), archive(threadApi, rootDirectory)
{
	HRSRC h = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_LUA_CORE), _T("LUA"));
	assert(h != nullptr);
    size_t size = ::SizeofResource(NULL, h);
	bootstrapCode.resize(size);
    HGLOBAL data = ::LoadResource(NULL, h);
	assert(data != 0);
    void* p = ::LockResource(data);
	memcpy(const_cast<char*>(bootstrapCode.c_str()), p, size);
	::UnlockResource(data);

	status.store(EXECUTIVE_RUNNING, std::memory_order_relaxed);

	uint32_t warpCount = GetThreadCount(); // 1:1 model
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
	workers.clear();
	::CloseHandle(iocpHandle);
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

void Executive::SetLogger(const TWrapper<void, const String&>& proc)
{
	logger = proc;
}

bool Executive::InvokeReset()
{
	uint32_t expected = EXECUTIVE_RUNNING;
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
	uint32_t expected = EXECUTIVE_RUNNING;
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
	uint32_t expected = EXECUTIVE_RESETTING;
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
	file->Release();
	file->ReleaseObject();
}

void Executive::OnScriptError(Worker& worker, IScript::Request& request, const String& err)
{

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
	worker.errorHandler = Wrap(this, &Executive::OnScriptError);

	IScript& script = worker.GetScript();
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	request << global << key("Executive") << *this << endtable; // register global functions
	IScript::Request::Ref ref = request.Load(bootstrapCode, "Core");
	request.Push();
	bool success = request.Call(ref);
	assert(success);
	request.Pop();
	request.Push();
	assert(!worker.compileRoutine);
	request << global << key("_CompileRoutine") >> worker.compileRoutine << endtable;
	assert(worker.compileRoutine);
	request.Pop();
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

	return *workers[warp];
}

TObject<IReflect>& Executive::operator () (IReflect& reflect)
{
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod())
	{
		ReflectMethod(RequestCompile)[ScriptMethodLocked = "Compile"];
		ReflectMethod(RequestDispatch)[ScriptMethodLocked = "Dispatch"];

		ReflectMethod(RequestQueryFiles)[ScriptMethodLocked = "QueryFiles"];
		ReflectMethod(RequestOpenFile)[ScriptMethodLocked = "OpenFile"];
		ReflectMethod(RequestCloseFile)[ScriptMethodLocked = "CloseFile"];
		ReflectMethod(RequestReadFile)[ScriptMethodLocked = "ReadFile"];
		ReflectMethod(RequestWriteFile)[ScriptMethodLocked = "WriteFile"];
		ReflectMethod(RequestGetFileSize)[ScriptMethodLocked = "GetFileSize"];
		ReflectMethod(RequestWriteOutput)[ScriptMethodLocked = "WriteOutput"];
		ReflectMethod(RequestUpdateStatus)[ScriptMethodLocked = "UpdateStatus"];
	}

	return *this;
}

TShared<ExecRoutine> Executive::RequestCompile(IScript::Request& request, IScript::Request::Ref func)
{
	CHECK_REFERENCES(func);

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
	CHECK_REFERENCES(callback);
	CHECK_DELEGATE(func);

	uint32_t targetWarp = kernel.PushIdleWarp();
	if (targetWarp != ~(uint32_t)0)
	{
		Worker& instance = *workers[targetWarp];
		assert(&instance != &GetCurrentWorker());

		const TShared<CrossRoutine>& routine = func.Get()->crossRoutines[targetWarp];
		if (routine)
		{
			instance.CallAsync(request, callback, routine, args);
			return true;
		}
		else
		{
			request.Error("Invalid queued function!");
		}
	}

	request.Dereference(callback);
	// all script engine busy, or target name not found
	return false;
}

void Executive::RequestQueryFiles(IScript::Request& request, StringView path)
{
	WIN32_FIND_DATAW findData;
	request << beginarray;
	HANDLE findHandle = ::FindFirstFileW((LPCWSTR)Utf8ToSystem(path).c_str(), &findData);
	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			String file = SystemToUtf8(String((const char*)findData.cFileName, wcslen(findData.cFileName) * 2));
			if (!!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				file += '/';
			}

			request << file;
		}
		while (::FindNextFileW(findHandle, &findData));

		::CloseHandle(findHandle);
	}
	request << endarray;
}

TShared<File> Executive::RequestOpenFile(IScript::Request& request, StringView path, uint32_t bufferSize, bool write)
{
	HANDLE fileHandle = ::CreateFileW((LPCWSTR)Utf8ToSystem(path).c_str(), write ? FILE_GENERIC_WRITE | FILE_GENERIC_READ : FILE_GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		File* file = fileAllocator.New(fileHandle, GetIocpHandle(), bufferSize);
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
	CHECK_REFERENCES(callback);
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
	CHECK_REFERENCES(callback);
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

void Executive::RequestWriteOutput(IScript::Request& request, IScript::Delegate<Document> document, StringView path, StringView content)
{
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(document);
	document->WriteOutput(path, content);
}

bool Executive::RequestUpdateStatus(IScript::Request& request, IScript::Delegate<Document> document, StringView status, StringView detail, float ratio)
{
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(document);

	return document->UpdateStatus(status, detail, ratio);
}

