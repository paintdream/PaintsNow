#include "PaintsNow.h"

#ifndef USE_RAW_MEMORY_ALIGNMENT

#if __STDCPP_DEFAULT_NEW_ALIGNMENT__ < 16
#include <malloc.h>
#include <stdlib.h>

#define DEFAULT_MEMORY_ALIGN 16 // default to 16, the alignment of most SSE/Neon instructions

void* operator new (size_t size) {
#ifdef _MSC_VER
	return _aligned_malloc(size, DEFAULT_MEMORY_ALIGN);
#else
	return memalign(DEFAULT_MEMORY_ALIGN, size);
#endif
}

void operator delete (void* ptr) {
#ifdef _MSC_VER
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}

void* operator new[] (size_t size) {
#ifdef _MSC_VER
	return _aligned_malloc(size, DEFAULT_MEMORY_ALIGN);
#else
	return memalign(DEFAULT_MEMORY_ALIGN, size);
#endif
}

void operator delete[] (void* ptr) {
#ifdef _MSC_VER
	_aligned_free(ptr);
#else
	free(ptr);
#endif	
}

#endif

#endif