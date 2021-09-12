// ITunnel.h
// PaintDream (paintdream@paintdream.com)
// 2016-7-5
//

#pragma once
#include "../../Core/Interface/IStreamBase.h"
#include "../../Core/Interface/IDevice.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Interface/IType.h"
#include <string>
#include <list>

namespace PaintsNow {
	class pure_interface ITunnel : public IDevice {
	public:
		~ITunnel() override;
		class Connection {};
		class Listener {};
		class Dispatcher {};

		virtual Dispatcher* OpenDispatcher() = 0;
		virtual bool ActivateDispatcher(Dispatcher* dispatcher) = 0;
		virtual void DeactivateDispatcher(Dispatcher* dispatcher) = 0;
		virtual Dispatcher* GetListenerDispatcher(Listener* listener) = 0;
		virtual Dispatcher* GetConnectionDispatcher(Connection* connection) = 0;
		virtual void CloseDispatcher(Dispatcher* dispatcher) = 0;

		enum EVENT { CONNECTED, TIMEOUT, READ, WRITE, CLOSE, ABORT, AWAKE, CUSTOM };
		virtual Listener* OpenListener(Dispatcher* dispatcher, const TWrapper<void, EVENT>& eventHandler, const TWrapper<const TWrapper<void, EVENT>, Connection*>& callback, const String& address) = 0;
		virtual bool ActivateListener(Listener* listener) = 0;
		virtual void GetListenerInfo(Listener* listener, String& address) = 0;
		virtual void DeactivateListener(Listener* listener) = 0;
		virtual void CloseListener(Listener* listener) = 0;

		virtual Connection* OpenConnection(Dispatcher* dispatcher, const TWrapper<void, EVENT>& callback, const String& address) = 0;
		virtual bool ActivateConnection(Connection* connection) = 0;
		virtual void GetConnectionInfo(Connection* connection, String& from, String& to) = 0;
		virtual void Flush(Connection* connection) = 0;
		virtual void Wakeup(Connection* connection) = 0;
		virtual bool ReadConnection(Connection* connection, void* data, size_t& length) = 0;
		virtual bool WriteConnection(Connection* connection, const void* data, size_t& length) = 0;
		virtual void DeactivateConnection(Connection* connection) = 0;
		virtual void CloseConnection(Connection* connection) = 0;

		typedef uint32_t PacketSizeType;
		struct PacketHeader {
			PacketHeader() : length(0) {}
			PacketSizeType length;
		};

		struct Packet {
			Packet() : cursor(0) {}
			PacketHeader header;
			PacketSizeType cursor;
		};

		virtual bool WriteConnectionPacket(Connection* connection, const void* data, PacketSizeType bufferLength, Packet& packet);
		virtual bool ReadConnectionPacket(Connection* connection, void* data, PacketSizeType& bufferLength, Packet& packet);

		typedef TWrapper<void, EVENT> Handler;
		typedef TWrapper<const Handler, Connection*> Notifier;
	};
}

