#include "Quota.h"
using namespace PaintsNow;

Quota::Quota(size_t size) : capacity(size) {
	allocated.store(0, std::memory_order_relaxed);
}

bool Quota::AcquireQuota(size_t count) {
	assert(count < capacity);
	size_t current = allocated.load(std::memory_order_relaxed);
	while (current + count < capacity) {
		if (allocated.compare_exchange_weak(current, current + count, std::memory_order_release)) {
			return true;
		}
	}

	return false;
}

size_t Quota::GetQuota() const {
	return capacity - allocated.load(std::memory_order_acquire);
}

size_t Quota::GetCapacity() const {
	return capacity;
}

void Quota::ReleaseQuota(size_t count) {
	assert(allocated.load(std::memory_order_acquire) >= count);
	allocated.fetch_sub(count, std::memory_order_relaxed);
}

