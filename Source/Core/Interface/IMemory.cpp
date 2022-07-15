#include "IMemory.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

static const size_t LARGE_PAGE = 64 * 1024;

namespace PaintsNow {
	namespace IMemory {
		void* AllocAligned(size_t size, size_t alignment) {
#ifdef _WIN32
			// 64K page, use low-level allocation
			if (size >= LARGE_PAGE && ((size & (LARGE_PAGE - 1)) == 0)) {
				assert(alignment <= LARGE_PAGE);
				return ::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE);
			} else {
				return _aligned_malloc(size, alignment);
			}
#else
			if (size >= LARGE_PAGE && ((size & (LARGE_PAGE - 1)) == 0)) {
				assert(alignment <= LARGE_PAGE);
				return mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			} else {
				return memalign(alignment, size);
			}
#endif
		}

		void FreeAligned(void* data, size_t size) {
#ifdef _WIN32
			if (size >= LARGE_PAGE && ((size & (LARGE_PAGE - 1)) == 0)) {
				::VirtualFree(data, 0, MEM_RELEASE);
			} else {
				_aligned_free(data);
			}
#else
			if (size >= LARGE_PAGE && ((size & (LARGE_PAGE - 1)) == 0)) {
				munmap(data, size);
			} else {
				free(data);
			}
#endif
		}
	}
}

