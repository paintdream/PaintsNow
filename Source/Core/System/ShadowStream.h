// ShadowStream.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-19
//

#pragma once
#include "../Interface/IType.h"
#include "../Interface/IStreamBase.h"
#include "../Template/TBuffer.h"

namespace PaintsNow {
	class ShadowStream : public IStreamBase {
	public:
		ShadowStream();
		~ShadowStream() override;

		bool operator << (IStreamBase& stream) override;
		bool operator >> (IStreamBase& stream) const override;

		bool Read(void* p, size_t& len) override;
		bool Write(const void* p, size_t& len) override;
		bool WriteDummy(size_t& len) override;
		bool Seek(SEEK_OPTION option, int64_t offset) override;
		void Flush() override;
		bool Transfer(IStreamBase& stream, size_t& len) override;

		Bytes& GetPayload();
		const Bytes& GetPayload() const;

		ShadowStream& operator = (const ShadowStream& localStream);
	
	protected:
		void Cleanup();

		IStreamBase* baseStream;
		uint64_t length;
		Bytes payload;
	};
}

