#include "stdafx.h"
using namespace PaintsNow;

// File
File::File(HANDLE file, HANDLE iocp, uint32_t bufferSize) : fileHandle(file), iocpHandle(iocp)
{
	buffer.resize(bufferSize);
	memset(&overlapped, 0, sizeof(overlapped));
	::CreateIoCompletionPort(file, iocpHandle, (ULONG_PTR)this, 0);
}

File::~File()
{
	Close();
}

HANDLE File::GetHandle() const
{
	return fileHandle;
}

const char* File::GetBuffer() const
{
	return buffer.c_str();
}

char* File::GetBuffer()
{
	return const_cast<char*>(buffer.c_str());
}

uint64_t File::GetSize() const
{
	LARGE_INTEGER li;
	if (::GetFileSizeEx(fileHandle, &li))
		return li.QuadPart;
	else
		return 0;
}

uint32_t File::GetBufferSize() const
{
	return (uint32_t)buffer.size();
}

OVERLAPPED* File::GetOverlapped()
{
	return &overlapped;
}

File* File::FromOverlapped(OVERLAPPED* overlapped)
{
	assert(overlapped != nullptr);
	return reinterpret_cast<File*>(reinterpret_cast<uint8_t*>(overlapped) - offsetof(File, overlapped));
}

bool File::Close()
{
	// locked operation, never release
	if (Acquire())
	{
		::CloseHandle(iocpHandle);
		::CloseHandle(fileHandle);
		return true;
	}
	else
	{
		return false;
	}
}

bool File::Acquire()
{
	return ::InterlockedExchange((volatile LONG*)&overlapped.hEvent, 1) == 0;
}

bool File::Release()
{
	return ::InterlockedExchange((volatile LONG*)&overlapped.hEvent, 0) == 1;
}

IScript::Request::Ref File::GetCallback() const
{
	return callback;
}

void File::SetCallback(const IScript::Request::Ref& cb)
{
	callback = cb;
}

