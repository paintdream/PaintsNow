// MemoryStream.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-19
//

#pragma once
#include "../Interface/IType.h"
#include "../Interface/IStreamBase.h"

namespace PaintsNow {
	// A resizable aligned memory block stream.
	class MemoryStream : public IStreamBase {
	public:
		MemoryStream(size_t maxSize, uint32_t alignment = 8);
		~MemoryStream() override;

		const void* GetBuffer() const;
		void* GetBuffer();

		void SetEnd();
		bool Read(void* p, size_t& len) override;
		bool Write(const void* p, size_t& len) override;
		bool WriteDummy(size_t& len) override;
		bool Seek(SEEK_OPTION option, int64_t offset) override;
		bool Truncate(uint64_t length) override;
		void Flush() override;
		size_t GetOffset() const;
		size_t GetTotalLength() const;
		size_t GetMaxLength() const;

		bool Transfer(IStreamBase& stream, size_t& len) override;
		IReflectObject* Clone() const override;
		bool Extend(size_t len);

	private:
		void Realloc(size_t newSize);
		bool CheckSize(size_t& len);

		uint8_t* buffer;
		size_t offset;
		size_t totalSize;
		size_t maxSize;

		uint32_t alignment;
	};
}

