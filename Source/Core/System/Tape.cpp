#include "Tape.h"
#include "../Template/TAlgorithm.h"
#include <cassert>

using namespace PaintsNow;

Tape::Tape(IStreamBase& s) : stream(s), location(0), maxLocation(0), lastOffset(0) {}

// get full length at given location
inline int Tape::FullLength(int64_t location, int64_t& length) {
	for (int c = 0, d; c < 32; c++) {
		if (c >= (d = (int)Math::SiteCount(location >> PACKAGE_MIN_ALIGN_LEVEL, (int64_t)((location + length + c * sizeof(PacketHeader) + PACKAGE_MIN_ALIGN - 1) >> PACKAGE_MIN_ALIGN_LEVEL)))) {
			length += c * sizeof(PacketHeader);
			return c - d;
		}
	}

	assert(false);
	return 0;
}

bool Tape::WritePacket(int64_t seq, IStreamBase& source, int64_t length) {
	// find aligned point
	int64_t fullLength = length + sizeof(length);
	int padding = FullLength(location, fullLength);
	PacketHeader header;
	header.firstMark = 1;
	header.padding = padding;
	header.seq = seq;

	int64_t alignedEnd = location + ((fullLength + PACKAGE_MIN_ALIGN - 1) & ~(PACKAGE_MIN_ALIGN - 1));
	int64_t mid = Math::AlignmentRange(location, alignedEnd);
	int64_t remaining = alignedEnd;

	// splitted into several segments
 	while (length > 0) {
		int64_t alignment = verify_cast<int64_t>(location < mid ? Math::Alignment(location | mid) : Math::AlignmentTop(remaining - mid));
		if (location >= mid) remaining -= alignment;

		assert(alignment != 0);
		// write atmost alignment - sizeof(header?) bytes
		int64_t extra = header.firstMark ? sizeof(length) : 0;
		int64_t full = alignment - sizeof(PacketHeader) - extra;
		int64_t size = Math::Min(length, full);

		header.offset = lastOffset;

		// write header
		size_t wl = sizeof(PacketHeader);
		if (!stream.Write((const uint8_t*)&header, wl)) {
			return false;
		}

		// write length for first packet
		int64_t len = verify_cast<int64_t>(fullLength);
		wl = sizeof(len);
		if (extra != 0 && !stream.Write((const uint8_t*)&len, wl)) {
			return false;
		}

		// prepare data
		wl = (size_t)size;
		if (!source.Transfer(stream, wl)) {
			return false;
		}

		header.firstMark = 0;
		length -= size;
		location += alignment;
		lastOffset = Math::Log2x((uint64_t)alignment);

		if (length == 0) {
			wl = (size_t)(alignedEnd - location + full - size);
			if (!stream.WriteDummy(wl)) {
				return false;
			}

			location = alignedEnd;
		}
	}

	maxLocation = location;
	return true;
}

bool Tape::ReadPacket(int64_t& seq, IStreamBase& target, int64_t& totalLength) {
	if (location >= maxLocation)
		return false;

	PacketHeader header;
	int64_t length = 0;
	totalLength = 0;
	int64_t alignedEnd = 0;
	int64_t mid = 0;
	int64_t remaining = 0;

	// read segments
	do {
		size_t rl = sizeof(PacketHeader);
		if (!stream.Read(&header, rl)) {
			return false;
		}

		if (length == 0) {
			assert(header.firstMark);
			rl = sizeof(length);
			if (!stream.Read(&length, rl)) {
				return false;
			} else {
				seq = header.seq;
				// totalLength = length;
				alignedEnd = location + ((length + PACKAGE_MIN_ALIGN - 1) & ~(PACKAGE_MIN_ALIGN - 1));
				length -= verify_cast<int64_t>(header.padding * sizeof(PacketHeader));
				mid = Math::AlignmentRange(location, alignedEnd);
				remaining = alignedEnd;
			}
		}

		int64_t alignment = verify_cast<int64_t>(location < mid ? Math::Alignment(location | mid) : Math::AlignmentTop(remaining - mid));
		if (location >= mid) remaining -= alignment;
		int64_t extra = header.firstMark ? sizeof(length) : 0;
		assert(length > (int64_t)sizeof(PacketHeader) + extra);
		int64_t size = Math::Min(length, alignment) - (sizeof(PacketHeader) + extra);

		assert((int64_t)size > 0);
		// read content
		rl = (size_t)size;
		if (!stream.Transfer(target, rl)) {
			return false;
		}

		if (!stream.Seek(IStreamBase::CUR, alignment - size - sizeof(PacketHeader) - extra)) {
			return false;
		}

		// commit bytes
		totalLength += size;
		length -= size + sizeof(PacketHeader) + extra;
		location += alignment;
		lastOffset = header.offset;
	} while (length > 0);

	return true;
}

bool Tape::Seek(int64_t seq) {
	if (maxLocation == 0)
		return true;

	int64_t left = 0, right = maxLocation;
	PacketHeader header;

	if (location == maxLocation) {
		// select step type
		int64_t diff = (int64_t)Math::Alignment(location);
		location -= diff;
		if (!stream.Seek(IStreamBase::CUR, -diff)) {
			return false;
		}
	}

	bool rewind = false;
	bool rewindDone = false;
	while (left < right) {
		size_t rl = sizeof(header);
		if (!stream.Read(&header, rl)) {
			return false;
		}

		// evaluate
		int64_t diff = (int64_t)Math::Alignment(location);

		if (header.seq < seq && !rewind) {
			// to right
			if (!rewindDone && diff != 0 && diff + location < right) {
				left = location;
				location += diff;
				if (!stream.Seek(IStreamBase::CUR, -(int64_t)sizeof(header) + diff)) {
					return false;
				}
			} else if (header.firstMark) {
				int64_t length;
				rl = sizeof(length);
				if (!stream.Read(&length, rl)) {
					return false;
				}

				int64_t alignedEnd = ((length + PACKAGE_MIN_ALIGN - 1) & ~(PACKAGE_MIN_ALIGN - 1));
				location += alignedEnd;
				if (!stream.Seek(IStreamBase::CUR, (int64_t)alignedEnd - sizeof(length) - sizeof(header))) {
					return false;
				}

				left = location;
			} else {
				if (!stream.Seek(IStreamBase::CUR, -(int64_t)sizeof(header))) {
					return false;
				}
				rewind = true;
				rewindDone = false;
			}
		} else {
			if (!rewind) {
				right = location;
				if (right == left) {
					if (!stream.Seek(IStreamBase::CUR, -(int64_t)sizeof(header))) {
						return false;
					}

					break;
				}
			}

			// to left
			if (rewind && header.firstMark) {
				if (!stream.Seek(IStreamBase::CUR, -(int64_t)sizeof(header))) {
					return false;
				}
				rewind = false;
				rewindDone = true;
			} else if (!rewind && location - diff >= left) {
				assert(diff != 0);
				location -= diff;
				if (!stream.Seek(IStreamBase::CUR, -(int64_t)sizeof(header) - diff)) {
					return false;
				}
			} else {
				// step back
				int64_t move = ((int64_t)1 << header.offset);
				assert((move & (PACKAGE_MIN_ALIGN - 1)) == 0);
				location -= move;

				if (!stream.Seek(IStreamBase::CUR, -(int64_t)sizeof(header) - move)) {
					return false;
				}
			}
		}
	}

	return true;
}

