// ConsoleStream.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-19
//

#pragma once
#include "../Interface/IType.h"
#include "../Interface/IStreamBase.h"
#include "../Template/TBuffer.h"

namespace PaintsNow {
	class ConsoleStream : public IStreamBase {
	public:
		ConsoleStream(FILE* fp = nullptr);
		~ConsoleStream() override;

		bool Read(void* p, size_t& len) override;
		bool Write(const void* p, size_t& len) override;
		bool WriteDummy(size_t& len) override;
		bool Seek(SEEK_OPTION option, int64_t offset) override;
		void Flush() override;
		bool Transfer(IStreamBase& stream, size_t& len) override;
		size_t Printf(const char* format, ...) override;

	private:
		FILE* fp;
#ifdef _WIN32
		void* consoleHandle;
#endif
	};
}

