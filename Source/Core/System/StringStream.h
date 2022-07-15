// StringStream.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-19
//

#pragma once
#include "../Interface/IType.h"
#include "../Interface/IStreamBase.h"

namespace PaintsNow {
	// A on-fly string-stream adapter, decorates a string like a stream.
	class StringStream : public IStreamBase {
	public:
		StringStream(String& str, int64_t pos = 0);
		~StringStream() override;

		bool Read(void* p, size_t& len) override;
		bool Write(const void* p, size_t& len) override;
		bool WriteDummy(size_t& len) override;
		bool Seek(SEEK_OPTION option, int64_t offset) override;
		void Flush() override;
		bool Transfer(IStreamBase& stream, size_t& len) override;
	
	protected:
		int64_t location;
		String& storage;
	};
}

