#include "ZDebuggerWin.h"
#include <cstring>
#if defined(_WIN32) || defined(WIN32)

#include <windows.h>

typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
	DWORD ThreadId;
	PEXCEPTION_POINTERS ExceptionPointers;
	BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef struct _MINIDUMP_IO_CALLBACK {
	HANDLE Handle;
	ULONG64 Offset;
	PVOID Buffer;
	ULONG BufferBytes;
} MINIDUMP_IO_CALLBACK, *PMINIDUMP_IO_CALLBACK;

typedef struct _MINIDUMP_READ_MEMORY_FAILURE_CALLBACK
{
	ULONG64 Offset;
	ULONG Bytes;
	HRESULT FailureStatus;
} MINIDUMP_READ_MEMORY_FAILURE_CALLBACK,
  *PMINIDUMP_READ_MEMORY_FAILURE_CALLBACK;

typedef struct _MINIDUMP_MEMORY_INFO {
	ULONG64 BaseAddress;
	ULONG64 AllocationBase;
	ULONG32 AllocationProtect;
	ULONG32 __alignment1;
	ULONG64 RegionSize;
	ULONG32 State;
	ULONG32 Protect;
	ULONG32 Type;
	ULONG32 __alignment2;
} MINIDUMP_MEMORY_INFO, *PMINIDUMP_MEMORY_INFO;

typedef struct _MINIDUMP_CALLBACK_OUTPUT {
	union {
	ULONG ModuleWriteFlags;
	ULONG ThreadWriteFlags;
	ULONG SecondaryFlags;
	struct {
		ULONG64 MemoryBase;
		ULONG MemorySize;
	};
	struct {
		BOOL CheckCancel;
		BOOL Cancel;
	};
	HANDLE Handle;
	struct {
		MINIDUMP_MEMORY_INFO VmRegion;
		BOOL Continue;
	};
	HRESULT Status;
	};
} MINIDUMP_CALLBACK_OUTPUT, *PMINIDUMP_CALLBACK_OUTPUT;

typedef struct _MINIDUMP_THREAD_CALLBACK {
	ULONG ThreadId;
	HANDLE ThreadHandle;
	CONTEXT Context;
	ULONG SizeOfContext;
	ULONG64 StackBase;
	ULONG64 StackEnd;
} MINIDUMP_THREAD_CALLBACK, *PMINIDUMP_THREAD_CALLBACK;

typedef struct _MINIDUMP_INCLUDE_MODULE_CALLBACK {
	ULONG64 BaseOfImage;
} MINIDUMP_INCLUDE_MODULE_CALLBACK, *PMINIDUMP_INCLUDE_MODULE_CALLBACK;

typedef struct _MINIDUMP_INCLUDE_THREAD_CALLBACK {
	ULONG ThreadId;
} MINIDUMP_INCLUDE_THREAD_CALLBACK, *PMINIDUMP_INCLUDE_THREAD_CALLBACK;

typedef struct _MINIDUMP_MODULE_CALLBACK {
	PWCHAR FullPath;
	ULONG64 BaseOfImage;
	ULONG SizeOfImage;
	ULONG CheckSum;
	ULONG TimeDateStamp;
	VS_FIXEDFILEINFO VersionInfo;
	PVOID CvRecord; 
	ULONG SizeOfCvRecord;
	PVOID MiscRecord;
	ULONG SizeOfMiscRecord;
} MINIDUMP_MODULE_CALLBACK, *PMINIDUMP_MODULE_CALLBACK;

typedef struct _MINIDUMP_THREAD_EX_CALLBACK {
	ULONG ThreadId;
	HANDLE ThreadHandle;
	CONTEXT Context;
	ULONG SizeOfContext;
	ULONG64 StackBase;
	ULONG64 StackEnd;
	ULONG64 BackingStoreBase;
	ULONG64 BackingStoreEnd;
} MINIDUMP_THREAD_EX_CALLBACK, *PMINIDUMP_THREAD_EX_CALLBACK;

typedef struct _MINIDUMP_CALLBACK_INPUT {
	ULONG ProcessId;
	HANDLE ProcessHandle;
	ULONG CallbackType;
	union {
	HRESULT Status;
	MINIDUMP_THREAD_CALLBACK Thread;
	MINIDUMP_THREAD_EX_CALLBACK ThreadEx;
	MINIDUMP_MODULE_CALLBACK Module;
	MINIDUMP_INCLUDE_THREAD_CALLBACK IncludeThread;
	MINIDUMP_INCLUDE_MODULE_CALLBACK IncludeModule;
	MINIDUMP_IO_CALLBACK Io;
	MINIDUMP_READ_MEMORY_FAILURE_CALLBACK ReadMemoryFailure;
	ULONG SecondaryFlags;
	};
} MINIDUMP_CALLBACK_INPUT, *PMINIDUMP_CALLBACK_INPUT;

typedef
BOOL
(WINAPI * MINIDUMP_CALLBACK_ROUTINE) (
	PVOID CallbackParam,
	PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
	);

typedef
BOOL
(WINAPI * MINIDUMP_CALLBACK_ROUTINE) (
	PVOID CallbackParam,
	PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
	);

typedef enum _MINIDUMP_TYPE {
	MiniDumpNormal			 = 0x00000000,
	MiniDumpWithDataSegs		   = 0x00000001,
	MiniDumpWithFullMemory		 = 0x00000002,
	MiniDumpWithHandleData		 = 0x00000004,
	MiniDumpFilterMemory		   = 0x00000008,
	MiniDumpScanMemory			 = 0x00000010,
	MiniDumpWithUnloadedModules		= 0x00000020,
	MiniDumpWithIndirectlyReferencedMemory = 0x00000040,
	MiniDumpFilterModulePaths		  = 0x00000080,
	MiniDumpWithProcessThreadData	  = 0x00000100,
	MiniDumpWithPrivateReadWriteMemory	 = 0x00000200,
	MiniDumpWithoutOptionalData		= 0x00000400,
	MiniDumpWithFullMemoryInfo		 = 0x00000800,
	MiniDumpWithThreadInfo		 = 0x00001000,
	MiniDumpWithCodeSegs		   = 0x00002000,
	MiniDumpWithoutAuxiliaryState	  = 0x00004000,
	MiniDumpWithFullAuxiliaryState	 = 0x00008000,
	MiniDumpWithPrivateWriteCopyMemory	 = 0x00010000,
	MiniDumpIgnoreInaccessibleMemory	   = 0x00020000,
	MiniDumpWithTokenInformation	   = 0x00040000,
	MiniDumpWithModuleHeaders		  = 0x00080000,
	MiniDumpFilterTriage		   = 0x00100000,
	MiniDumpValidTypeFlags		 = 0x001fffff,
} MINIDUMP_TYPE;

typedef struct _MINIDUMP_USER_STREAM {
	ULONG32 Type;
	ULONG BufferSize;
	PVOID Buffer;

} MINIDUMP_USER_STREAM, *PMINIDUMP_USER_STREAM;

typedef struct _MINIDUMP_USER_STREAM_INFORMATION {
	ULONG UserStreamCount;
	PMINIDUMP_USER_STREAM UserStreamArray;
} MINIDUMP_USER_STREAM_INFORMATION, *PMINIDUMP_USER_STREAM_INFORMATION;

typedef struct _MINIDUMP_CALLBACK_INFORMATION {
	MINIDUMP_CALLBACK_ROUTINE CallbackRoutine;
	PVOID CallbackParam;
} MINIDUMP_CALLBACK_INFORMATION, *PMINIDUMP_CALLBACK_INFORMATION;

typedef
BOOL
(WINAPI *
pfnMiniDumpWriteDump)(
	HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	DWORD DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

pfnMiniDumpWriteDump MiniDumpWriteDump = nullptr;

#endif // WINDOWS

using namespace PaintsNow;

#if defined(_WIN32) || defined(WIN32)

static ZDebuggerWin* dump = nullptr;

static DWORD _stdcall DumpThread(LPVOID param) {
	PEXCEPTION_POINTERS pep = (PEXCEPTION_POINTERS)param;
	HANDLE hFile = ::CreateFileW((LPCWSTR)SystemToUtf8(dump->path).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if ((hFile != nullptr) && (hFile != INVALID_HANDLE_VALUE)) {
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = TRUE;

		if (MiniDumpWriteDump != nullptr) {
			fprintf(stderr, "Exception caught. Writing MiniDump ...\n");

			if (!::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpNormal | MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithHandleData | MiniDumpWithDataSegs | MiniDumpWithCodeSegs | MiniDumpWithModuleHeaders, nullptr, nullptr, nullptr)) {
				fprintf(stderr, "Dump failed! Error code = %x\n", ::GetLastError());
			}
		}

		CloseHandle(hFile);
	}

	return 0;
}

static long _stdcall OnException(PEXCEPTION_POINTERS pep) {
	if (dump != nullptr && (!dump->handler || dump->handler())) {
		HANDLE dumpThread = ::CreateThread(NULL, 0, DumpThread, pep, 0, NULL);
		::WaitForSingleObject(dumpThread, INFINITE);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

#endif // _WIN32

ZDebuggerWin::ZDebuggerWin() {
#if defined(_WIN32) || defined(WIN32)
	assert(dump == nullptr);
	HMODULE hModule = ::LoadLibraryW(L"dbghelp.dll");
	if (hModule != nullptr) {
		MiniDumpWriteDump = (pfnMiniDumpWriteDump)::GetProcAddress(hModule, "MiniDumpWriteDump");
		dump = this;
		::SetUnhandledExceptionFilter(OnException);
	}
#endif
}

ZDebuggerWin::~ZDebuggerWin() {
#if defined(_WIN32) || defined(WIN32)
	dump = nullptr;
#endif
}

void ZDebuggerWin::SetDumpHandler(const String& p, const TWrapper<bool>& w) {
	handler = w;
	path = p;
}

void ZDebuggerWin::StartDump(const String& options) {
#if defined(_WIN32) || defined(WIN32)
	DebugBreak();
#endif
}

void ZDebuggerWin::EndDump() {

}

void ZDebuggerWin::InvokeDump(const String& options) {
	StartDump(options);
}