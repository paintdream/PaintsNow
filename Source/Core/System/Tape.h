// Tape.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-8
//

#pragma once 

#include "../Interface/IStreamBase.h"
#include "../Interface/IType.h"

namespace PaintsNow {
	// A timestamp/sequence-id tagged data recorder
	class Tape {
	private:
		struct PacketHeader {
			int64_t firstMark : 1;
			int64_t padding : 5;
			int64_t offset : 5; // log2(32)
			int64_t seq : 53; // 64 - firstMark - offset - padding
			// int64_t length; // if firstMark = 1
		};

		// 32-bit version
		/* struct PacketHeader {
			uint32_t firstMark : 1;
			uint32_t offsetType : 1;
			uint32_t offset : 5;
			uint32_t seq : 25;
		};*/

	public:
		Tape(IStreamBase& stream);

		enum { PACKAGE_MIN_ALIGN_LEVEL = 5 };
		enum { PACKAGE_MIN_ALIGN = 1 << (PACKAGE_MIN_ALIGN_LEVEL) };

		bool ReadPacket(int64_t& seq, IStreamBase& target, int64_t& length);
		bool WritePacket(int64_t seq, IStreamBase& source, int64_t length);
		bool Seek(int64_t seq);

	protected:
		int FullLength(int64_t location, int64_t& length);

	protected:
		IStreamBase& stream;
		char lastOffset;
		char reserved[3];
		int64_t location;
		int64_t maxLocation;
	};
}
