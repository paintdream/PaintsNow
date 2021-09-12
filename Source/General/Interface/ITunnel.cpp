#include "ITunnel.h"

using namespace PaintsNow;

ITunnel::~ITunnel() {}

bool ITunnel::ReadConnectionPacket(Connection* c, void* data, PacketSizeType& bufferLength, Packet& packet) {
	size_t availableBytes = 0;
	assert(bufferLength != 0);

	// check new packet
	if (packet.cursor == packet.header.length) {
		packet.header.length = packet.cursor = 0;
	}

	if (packet.header.length == 0) {
		// How many bytes can I read ?
		ReadConnection(c, nullptr, availableBytes);
		if (availableBytes < sizeof(packet.header)) {
			return false; // not enough data
		}

		availableBytes = sizeof(packet.header);
		if (!ReadConnection(c, &packet.header, availableBytes)) {
			return false;
		}

		assert(availableBytes == sizeof(packet.header));
	}

	PacketSizeType remaining = packet.header.length - packet.cursor;
	availableBytes = 0;
	ReadConnection(c, nullptr, availableBytes);

	// read as most as possible
	availableBytes = Math::Min((PacketSizeType)availableBytes, Math::Min(remaining, bufferLength));
	if (availableBytes != 0 && ReadConnection(c, data, availableBytes)) {
		// Complete! 
		packet.cursor += (PacketSizeType)availableBytes;
		bufferLength = (PacketSizeType)availableBytes;
		return true;
	} else {
		return false;
	}
}

bool ITunnel::WriteConnectionPacket(Connection* c, const void* data, PacketSizeType bufferLength, Packet& packet) {
	// write data length first, then hash, finally data
	assert(bufferLength != 0 && (bufferLength + packet.cursor <= packet.header.length));

	size_t length = sizeof(packet.header);
	if (packet.cursor == 0) {
		PacketHeader endianCopy = packet.header;
		if (!WriteConnection(c, &endianCopy, length)) {
			return false;
		}
	}

	length = bufferLength;
	if (!WriteConnection(c, data, length)) {
		return false;
	} else {
		packet.cursor += (PacketSizeType)length;
		return true;
	}
}
