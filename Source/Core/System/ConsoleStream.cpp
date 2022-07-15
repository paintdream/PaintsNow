#include "ConsoleStream.h"
#include <cstdio>
#include <cstdarg>

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
static CRITICAL_SECTION consoleLock;

class CriticalSectionInitializer {
public:
	CriticalSectionInitializer() {
		::InitializeCriticalSection(&consoleLock);
	}
};

#endif

using namespace PaintsNow;

ConsoleStream::ConsoleStream(FILE* f) : fp(f) {
#ifdef _WIN32
	TSingleton<CriticalSectionInitializer>::Get();

	consoleHandle = ::GetStdHandle(fp == stderr ? STD_ERROR_HANDLE : fp == stdin ? STD_INPUT_HANDLE : STD_OUTPUT_HANDLE);
#endif
}

ConsoleStream::~ConsoleStream() {}

size_t ConsoleStream::Printf(const char* format, ...) {
	va_list va;
	va_start(va, format);
	const char* p = format;
#ifdef _WIN32
	::EnterCriticalSection(&consoleLock);

	// parse color
	if (format[0] == '#') {
		p = strchr(format + 1, '#');
		if (p == nullptr || p - format < 7) {
			::LeaveCriticalSection(&consoleLock);
			va_end(va);
			return 0;
		}

		WORD attribute = FOREGROUND_INTENSITY;
		if (format[1] != '0' && format[2] != '0') {
			attribute |= FOREGROUND_RED;
		}

		if (format[3] != '0' && format[4] != '0') {
			attribute |= FOREGROUND_GREEN;
		}

		if (format[5] != '0' && format[6] != '0') {
			attribute |= FOREGROUND_BLUE;
		}

		p++;
		::SetConsoleTextAttribute(consoleHandle, attribute);
	} else if (fp == stderr) {
		::SetConsoleTextAttribute(consoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED);
	}
#endif

	int n = vfprintf(fp, p, va);

#ifdef _WIN32
	if (format[0] == '#' || fp == stderr) {
		fflush(fp);
		::SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}

	::LeaveCriticalSection(&consoleLock);
#endif

	va_end(va);

	return n < 0 ? 0 : (size_t)n;
}

bool ConsoleStream::Read(void* p, size_t& len) {
	return (len = fread(p, 1, len, fp)) != 0;
}

bool ConsoleStream::Write(const void* p, size_t& len) {
	return (len = fwrite(p, 1, len, fp)) != 0;
}

bool ConsoleStream::Seek(SEEK_OPTION option, int64_t offset) {
	int s = SEEK_CUR;
	switch (option) {
		case BEGIN:
			s = SEEK_SET;
			break;
		case END:
			s = SEEK_END;
			break;
		case CUR:
			s = SEEK_CUR;
			break;
	}

	return fseek(fp, (long)offset, s) == 0;
}

bool ConsoleStream::Transfer(IStreamBase& stream, size_t& len) {
	const size_t SIZE = 512;
	char buffer[SIZE];
	size_t rl = Math::Min(SIZE, len);
	while (len > 0 && Read(buffer, rl)) {
		size_t wl = Math::Min(SIZE, len);
		stream.Write(buffer, wl);
		len -= SIZE;
		rl = Math::Min(SIZE, len);
	}

	return len == 0;
}

bool ConsoleStream::WriteDummy(size_t& len) {
	return fseek(fp, (long)len, SEEK_CUR) == 0;
}

void ConsoleStream::Flush() {
	fflush(fp);
}