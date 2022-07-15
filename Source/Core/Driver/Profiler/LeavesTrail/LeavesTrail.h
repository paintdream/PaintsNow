// LeavesTrail.h
// PaintDream (paintdream@paintdream.com)
// 2021-04-01
//

#pragma once

#include <string>
#include <vector>

#if defined(_MSC_VER) && _MSC_VER <= 1200
#include "../../../Interface/IType.h"
#include "../../../Template/TAtomic.h"
#else
#include <atomic>
using String = std::string;
#endif

namespace PaintsNow {
	class LeavesTrail {
	public:
		enum { DEFAULT_EVENT_COUNT = 6400 };
		LeavesTrail(const String& typeName, size_t size, size_t alignment, size_t maxEventCount = DEFAULT_EVENT_COUNT);

		void AddSymbol(size_t offset, size_t size, const String& symbolType, const String& symbolName);
		void AddInstance(void* instance);
		void SetLogPath(const String& path);

		void Start();
		void Tick();
		void Stop();

		enum BREAK_TYPE { EXECUTE, READWRITE, WRITE };
		static void SetHardwareBreakpoint(size_t count, void* addresses[], size_t length, BREAK_TYPE type, size_t* slots);
		size_t GetTypeSize() const;
		const String& GetTypeName() const;

	public:
		void CaptureEvent(void* address);

	protected:
		void UpdateBreakpoints();
		void CaptureTick();
		void Report();

		class Symbol {
		public:
			Symbol(size_t offset, size_t size, const String& type, const String& name);
			bool operator < (const Symbol& rhs) const { return offset < rhs.offset; }
			size_t offset;
			size_t size;
			String type;
			String name;
		};

		class Event {
		public:
			Event();
			Event(size_t offset, size_t instanceID, size_t threadIndex);
			Event(uint64_t timestamp);

			union {
				struct {
					uint16_t isTimestamp : 1;
					uint16_t offset : 15;
					uint16_t instanceID;
					uint32_t threadIndex;
				};

				struct {
					uint64_t isTimestampReserved : 1;
					uint64_t timestamp : 63;
				};
			};
		};

		std::atomic<size_t> status;
		std::atomic<size_t> eventIndex;
		size_t typeSize;
		size_t typeAlignment;

		String typeName;
		String logPath;
		std::vector<void*> instances;
		std::vector<Symbol> symbols;
		std::vector<Event> events;
	};
}