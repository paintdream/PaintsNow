// IMemory -- Basic memory allocator interface
// PaintDream (paintdream@paintdream.com)
// 2014-12-3
//

#pragma once
#include "../PaintsNow.h"
#include "../Template/TAtomic.h"
#include <malloc.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#if defined(_M_AMD64) || defined(_M_IX86)
#include <xmmintrin.h>
#include <emmintrin.h>
#else
#include <arm_neon.h>
#endif
#endif

namespace PaintsNow {
	namespace IMemory {
		template <class T>
		class ObjectLeakGuard {
		public:
#ifdef _DEBUG
			class Finalizer {
			public:
				Finalizer(std::atomic<int32_t>& c) : counter(c) {}
				~Finalizer() {
					assert(counter.load(std::memory_order_relaxed) == 0);
				}

				std::atomic<int32_t>& counter;
			};

			ObjectLeakGuard() {
				++GetCounter();
			}

			~ObjectLeakGuard() {
				--GetCounter();
			}

			static std::atomic<int32_t>& GetCounter() {
				static std::atomic<int32_t> counter;
				static Finalizer finalizer(counter);
				return counter;
			}
#endif
		};

		static inline void PrefetchRead(const void* address) {
#ifdef _MSC_VER
#if defined(_M_IX86) || defined(_M_AMD64)
			_mm_prefetch(reinterpret_cast<const char*>(address), _MM_HINT_NTA);
#else
			// __prefetch(address); // not exist on MSVC in fact
#endif
#else
			__builtin_prefetch(address, 0, 0);
#endif
		}

		static inline void PrefetchReadLocal(const void* address) {
#ifdef _MSC_VER
#if defined(_M_AMD64) || defined(_M_IX86)
			_mm_prefetch(reinterpret_cast<const char*>(address), _MM_HINT_T0);
#endif
#else
			__builtin_prefetch(address, 0, 3);
#endif
		}

		static inline void PrefetchWrite(void* address) {
#ifdef _MSC_VER
#if defined(_M_AMD64) || defined(_M_IX86)
			_mm_prefetch(reinterpret_cast<char*>(address), _MM_HINT_NTA);
#endif
#else
			__builtin_prefetch(address, 1, 0);
#endif
		}

		static inline void PrefetchWriteLocal(void* address) {
#ifdef _MSC_VER
#if defined(_M_AMD64) || defined(_M_IX86)
			_mm_prefetch(reinterpret_cast<char*>(address), _MM_HINT_T0);
#endif
#else
			__builtin_prefetch(address, 1, 3);
#endif
		}

		void* AllocAligned(size_t size, size_t alignment);
		void FreeAligned(void* data, size_t size);
	};
}

