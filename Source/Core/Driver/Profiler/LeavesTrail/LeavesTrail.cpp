#include "LeavesTrail.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <cstdlib>

#if !defined(_MSC_VER) || _MSC_VER > 1200
#include <random>
#endif

using namespace PaintsNow;
static const size_t BREAKPOINT_COUNT = 4;
static const size_t CACHELINE_SIZE = 64;

#if defined(_WIN32) && defined(_M_IX86) || defined(_M_AMD64)
#include <windows.h>
#include <tlhelp32.h>
#if _MSC_VER > 1200
#include <intrin.h>
#endif
static LeavesTrail* theLeavesTrail = nullptr;
static void* VecHandle = nullptr;
static uint64_t performanceTick = 0;

static LONG _stdcall VectoredExceptionHandler(_EXCEPTION_POINTERS* ExceptionInfo) {
	if (theLeavesTrail != nullptr) {
		if (ExceptionInfo->ExceptionRecord->ExceptionCode & EXCEPTION_BREAKPOINT) {
			// Get visited memory address
			CONTEXT* context = ExceptionInfo->ContextRecord;
			size_t dr6 = context->Dr6;
			for (size_t i = 0; i < BREAKPOINT_COUNT; i++) {
				if (dr6 & ((size_t)1 << i)) {
					theLeavesTrail->CaptureEvent(((void**)&context->Dr0)[i]);
				}
			}
		}
	}
	
	return EXCEPTION_CONTINUE_EXECUTION;
}

static uint64_t GetCurrentTimestamp() {
	LARGE_INTEGER ret;
	::QueryPerformanceCounter(&ret);
	return (((uint64_t)ret.HighPart << 31) << 1) | ret.LowPart;
}

static uint32_t GetCurrentThreadIndex() {
	return ::GetCurrentThreadId();
}

static void StartVectoredExceptionHandler(LeavesTrail* trail) {
	assert(theLeavesTrail == nullptr);
	theLeavesTrail = trail;
	VecHandle = ::AddVectoredExceptionHandler(0, VectoredExceptionHandler);
}

static void StopVectoredExceptionHandler(LeavesTrail* trail) {
	assert(theLeavesTrail != nullptr);
	::RemoveVectoredExceptionHandler(VecHandle);
	VecHandle = nullptr;
	theLeavesTrail = nullptr;
	std::atomic_thread_fence(std::memory_order_release);
}

#else

static void StartVectoredExceptionHandler(LeavesTrail* trail) {
	// TODO:
}

static void StopVectoredExceptionHandler(LeavesTrail* trail) {
	// TODO:
}

static uint64_t GetCurrentTimestamp() {
	// TODO:
	return 0;
}

static uint32_t GetCurrentThreadIndex() {
	// TODO:
	return 0;
}

static uint64_t performanceTick = 1000 * 1000;
#endif

LeavesTrail::Symbol::Symbol(size_t off, size_t s, const String& tp, const String& n) : offset(off), size(s), type(tp), name(n) {}
LeavesTrail::Event::Event() {}
LeavesTrail::Event::Event(size_t off, size_t instID, size_t thread) : isTimestamp(0), offset((uint16_t)off), instanceID((uint16_t)instID), threadIndex((uint32_t)thread) {}
LeavesTrail::Event::Event(uint64_t ts) : isTimestampReserved(1), timestamp(ts) {}

LeavesTrail::LeavesTrail(const String& name, size_t size, size_t align, size_t maxEventCount) : typeName(name), typeSize(size), typeAlignment(align) {
#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_AMD64))
	LARGE_INTEGER v;
	::QueryPerformanceFrequency(&v);
	performanceTick = ((v.HighPart << 31) << 1) | v.LowPart;
#endif
	events.resize(maxEventCount);
	status.store(0, std::memory_order_relaxed);
	eventIndex.store(0, std::memory_order_release);
}

void LeavesTrail::AddSymbol(size_t offset, size_t size, const String& type, const String& name) {
	assert(status.load(std::memory_order_acquire) == 0);
	symbols.emplace_back(Symbol(offset, size, type, name));
}

void LeavesTrail::AddInstance(void* instance) {
	assert(status.load(std::memory_order_acquire) == 0);
	instances.emplace_back(instance);
}

void LeavesTrail::Start() {
	assert(status.load(std::memory_order_acquire) == 0);
	status.store(1, std::memory_order_release);
	std::sort(instances.begin(), instances.end());
	std::sort(symbols.begin(), symbols.end());
	StartVectoredExceptionHandler(this);
}

void LeavesTrail::Stop() {
	if (status.exchange(0, std::memory_order_acquire) != 0) {
		UpdateBreakpoints();
		StopVectoredExceptionHandler(this);
		Report();
	}
}

void LeavesTrail::CaptureEvent(void* address) {
	std::vector<void*>::iterator it = std::upper_bound(instances.begin(), instances.end(), address);
	assert(!instances.empty());
	if (it == instances.begin()) return;
	--it;

	size_t base = reinterpret_cast<size_t>(*it);
	size_t offset = reinterpret_cast<size_t>(address) - base;
	if (offset > typeSize) return;

	size_t index = eventIndex.fetch_add(1, std::memory_order_acq_rel);
	if (index < events.size()) {
		events[index] = Event(offset, it - instances.begin(), GetCurrentThreadIndex());
	} else if (index == events.size()) {
		events.back() = Event(GetCurrentTimestamp());
	}
}

size_t LeavesTrail::GetTypeSize() const {
	return typeSize;
}

const String& LeavesTrail::GetTypeName() const {
	return typeName;
}

void LeavesTrail::CaptureTick() {
	size_t index = eventIndex.fetch_add(1, std::memory_order_acq_rel);
	if (index < events.size()) {
		events[index] = Event(GetCurrentTimestamp());
	}
}

static String FilterType(const String& v) {
	String s = v;
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] == '<') s[i] = '[';
		else if (s[i] == '>') s[i] = ']';
	}

	return s;
}

void LeavesTrail::Report() {
	if (!logPath.empty()) {
		std::ofstream logFile;
		logFile.open(logPath.c_str(), std::ios_base::binary);
		if (logFile.good()) {
			logFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
			logFile << "<LeavesTrail>\n";
			logFile << "\t<Events>\n";
			size_t k = 0;
			uint64_t startTime = 0;
			size_t hitCount = 0;
			std::vector<size_t> symbolTransitionMatrix;
			symbolTransitionMatrix.resize(symbols.size() * symbols.size());
			std::vector<size_t> symbolTransitionMatrixSharing;
			symbolTransitionMatrixSharing.resize(symbols.size() * symbols.size());

			size_t alignTypeCount = CACHELINE_SIZE > typeAlignment ? CACHELINE_SIZE / typeAlignment : 1;
			std::vector<size_t> symbolHitCounts;
			symbolHitCounts.resize(symbols.size());
			for (size_t i = 0; i < events.size(); i++) {
				const Event& ev = events[i];
				size_t lastSymbol = ~(size_t)0;
				size_t lastThread = ~(size_t)0;

				if (ev.isTimestamp) {
					if (startTime == 0) startTime = ev.timestamp;
					if (k == i) continue; // no events? skip

					logFile << "\t\t<Node time=\"" << (uint32_t)((ev.timestamp - startTime) * 1000 * 1000 / performanceTick) << "\">\n";
					for (size_t n = k; n < i; n++) {
						const Event& e = events[n];
						// find symbol
						String symbol;
						if (!symbols.empty()) {
							Symbol sym(e.offset, 0, "", "");
							std::vector<Symbol>::iterator it = std::upper_bound(symbols.begin(), symbols.end(), sym);
							if (it != symbols.begin()) {
								--it;

								assert(it->offset <= e.offset);
								if (e.offset < it->offset + it->size) {
									size_t currentSymbol = it - symbols.begin();
									symbolHitCounts[currentSymbol]++;
									symbol = it->name;
									if (lastSymbol != ~(size_t)0) {
										symbolTransitionMatrix[lastSymbol * symbols.size() + currentSymbol] += alignTypeCount;

										if (lastThread != ~(size_t)0 && lastThread != e.threadIndex) {
											// test false-sharing
											size_t start, end;
											if (symbols[currentSymbol].offset > symbols[lastSymbol].offset) {
												start = symbols[lastSymbol].offset + symbols[lastSymbol].size - 1;
												end = symbols[currentSymbol].offset;
											} else {
												start = symbols[currentSymbol].offset + symbols[currentSymbol].size - 1;
												end = symbols[lastSymbol].offset;
											}

											const size_t CACHELINE_MASK = ~(CACHELINE_SIZE - 1);
											size_t coherentCount = 0;
											for (size_t i = 0; i < alignTypeCount; i++) {
												if ((start & CACHELINE_MASK) == (end & CACHELINE_MASK)) {
													coherentCount++;
												}

												start += typeAlignment;
												end += typeAlignment;
											}

											symbolTransitionMatrixSharing[lastSymbol * symbols.size() + currentSymbol] += coherentCount;
										}
									}
									
									lastSymbol = currentSymbol;
								} else {
									symbol = it->name + "?";
									lastSymbol = ~(size_t)0;
								}

								hitCount++;
							}
						}

						lastThread = e.threadIndex;

						logFile << "\t\t\t<Event thread=\"" << e.threadIndex << "\" instance=\"" << e.instanceID << "\" offset=\"" << e.offset << "\" symbol=\"" << symbol << "\"/>\n";
					}
					logFile << "\t\t</Node>\n";
					k = i + 1;
				}
			}
			logFile << "\t</Events>\n";
			logFile << "\t<Symbols type=\"" << typeName << "\" size=\"" << typeSize << "\" hit=\"" << hitCount << "\">\n";
			for (size_t m = 0; m < symbols.size(); m++) {
				const Symbol& symbol = symbols[m];
				logFile << "\t\t<Symbol name=\"" << symbol.name << "\" type=\"" << FilterType(symbol.type) << "\" offset=\"" << symbol.offset << "\" hit=\"" << symbolHitCounts[m] << "\" />\n";
			}
			logFile << "\t</Symbols>\n";
			logFile << "\t<SymbolTransitionMatrix>\n";
			for (size_t w = 0; w < symbols.size(); w++) {
				const Symbol& symbol = symbols[w];
				logFile << "\t\t<From name=\"" << symbol.name << "\">\n";
				size_t totalHit = 0;
				for (size_t t = 0; t < symbols.size(); t++) {
					totalHit += symbolTransitionMatrix[w * symbols.size() + t];
				}

				for (size_t n = 0; n < symbols.size(); n++) {
					float transition = totalHit == 0 ? 0.0f : (float)symbolTransitionMatrix[w * symbols.size() + n] / totalHit;
					float sharingRatio = symbolTransitionMatrix[w * symbols.size() + n] == 0 ? 0.0f : symbolTransitionMatrixSharing[w * symbols.size() + n] / (float)symbolTransitionMatrix[w * symbols.size() + n];
					logFile << "\t\t\t<To name=\"" << symbols[n].name << "\" value=\"" << transition << "\" sharing=\"" << sharingRatio << "\" />\n";
				}

				logFile << "\t\t</From>\n";
			}
			logFile << "\t</SymbolTransitionMatrix>\n";
			logFile << "</LeavesTrail>\n";
			logFile.close();
		}
	}
}

void LeavesTrail::SetLogPath(const String& path) {
	logPath = path;
}

void LeavesTrail::UpdateBreakpoints() {
	void* addresses[BREAKPOINT_COUNT] = { nullptr };

	if (status.load(std::memory_order_acquire) != 0) {
		// randomly select instances to capture
		size_t index = rand() % instances.size();
		size_t varCount = (typeSize + sizeof(size_t) - 1) / sizeof(size_t);

		size_t n = 0;

		size_t instance = (size_t)instances[rand() % instances.size()];
		size_t count = varCount < BREAKPOINT_COUNT ? varCount : BREAKPOINT_COUNT;
		std::vector<size_t> offset;
		offset.reserve(varCount);
		for (size_t i = 0; i < varCount; i++) {
			offset.emplace_back(i);
		}

		// deprecated in C++17
		#if defined(_MSC_VER) && _MSC_VER <= 1200
		std::random_shuffle(offset.begin(), offset.end());
		#else
		static int seed = 0;
		std::shuffle(offset.begin(), offset.end(), std::default_random_engine(seed++));
		#endif
		for (size_t k = 0; k < count; k++) {
			addresses[k] = reinterpret_cast<void*>((size_t)instances[index] + offset[k] * sizeof(size_t));
		}
	}

	SetHardwareBreakpoint(4, addresses, sizeof(size_t), READWRITE, nullptr);
}

void LeavesTrail::Tick() {
	// Must be called in an external thread
	CaptureTick();
	UpdateBreakpoints();
}

void LeavesTrail::SetHardwareBreakpoint(size_t count, void* addresses[], size_t length, BREAK_TYPE type, size_t* slots) {
#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_AMD64))
	assert(count <= 4);
	size_t flag = 0;
	size_t mask = 0;
	for (size_t i = 0; i < count; i++) {
		size_t slot = slots == nullptr ? i : slots[i];
		assert(slot < 4);
		size_t sflag = (1
			| (type == LeavesTrail::EXECUTE ? 0 : type == LeavesTrail::WRITE ? 0x10000 : 0x30000)
			| (length == 8 ? 0x2000000 : length == 4 ? 0x3000000 : length == 2 ? 0x1000000 : 0)) << (slot * 2);

		// clear debug point?
		if (addresses[i] == nullptr) {
			sflag = 0;
		}

		flag |= sflag;
		mask |= (0x3 | 0x30000 | 0x3000000) << (slot * 2);
	}

	HANDLE h = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	DWORD currentThreadID = ::GetCurrentThreadId();
	DWORD currentProcessID = ::GetCurrentProcessId();
	if (h != INVALID_HANDLE_VALUE) {
		THREADENTRY32 te;
		te.dwSize = sizeof(THREADENTRY32);
		if (::Thread32First(h, &te)) {
			do {
				if (te.th32OwnerProcessID == currentProcessID && te.th32ThreadID != currentThreadID) {
					HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
					if (hThread != INVALID_HANDLE_VALUE) {
						::SuspendThread(hThread);

						CONTEXT context;
						context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

						if (::GetThreadContext(hThread, &context)) {
							context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
							for (size_t i = 0; i < count; i++) {
								((void**)&context.Dr0)[slots == nullptr ? i : slots[i]] = addresses[i];
							}

							context.Dr7 = ((context.Dr7 & ~mask) | flag);
							::SetThreadContext(hThread, &context);
						}

						::ResumeThread(hThread);
						::CloseHandle(hThread);
					} else {
						assert(false); // not possible!
					}
				}
			} while (::Thread32Next(h, &te));
		}

		::CloseHandle(h);

		// OK, then ourself
		// These functions are only available in kernel mode.
		/*
#if _MSC_VER <= 1200
		// no intrin.h available, go inline asm
		for (size_t i = 0; i < count; i++) {
			void* addr = addresses[i];
			switch (i) {
			case 0:
				_asm {
					push eax;
					mov eax, addr;
					mov dr0, eax;
					pop eax;
				}
				break;
			case 1:
				_asm {
					push eax;
					mov eax, addr;
					mov dr1, eax;
					pop eax;
				}
				break;
			case 2:
				_asm {
					push eax;
					mov eax, addr;
					mov dr2, eax;
					pop eax;
				}
				break;
			case 3:
				_asm {
					push eax;
					mov eax, addr;
					mov dr3, eax;
					pop eax;
				}
				break;
			}
		}

		size_t invmask = ~mask;
		_asm {
			push eax;
			mov eax, dr7;
			and eax, invmask;
			or eax, flag;
			mov dr7, eax;
			pop eax;
		}
#else
		for (size_t i = 0; i < count; i++) {
			switch (i) {
			case 0:
				__writedr(0, (size_t)addresses[i]);
				break;
			case 1:
				__writedr(1, (size_t)addresses[i]);
				break;
			case 2:
				__writedr(2, (size_t)addresses[i]);
				break;
			case 3:
				__writedr(3, (size_t)addresses[i]);
				break;
			}
		}

		__writedr(7, ((__readdr(7) & ~mask) | flag));
#endif
*/
	}
#endif
}
