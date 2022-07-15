// TPool.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-17
//

#pragma once
#include "../PaintsNow.h"
#include "TAtomic.h"
#include <stack>

namespace PaintsNow {
	// A single object pool
	template <class T, class Allocator, size_t N = 8>
	class TPool : protected TEnableReadWriteFence<> {
	public:
		typedef TPool PoolBase;
		TPool(Allocator& alloc, uint32_t count) : allocator(alloc), maxCount(count), currentCount(0) {
			memset(heads, 0, sizeof(heads));
		}

		~TPool() {
			Clear();
		}

		TPool(rvalue<TPool> rhs) : allocator(((TPool&)rhs).allocator), maxCount(((TPool&)rhs).maxCount), currentCount(((TPool&)rhs).currentCount) {
			memcpy(heads, (TPool&)rhs.heads, sizeof(heads));
			memset((TPool&)rhs.heads, 0, sizeof(rhs.heads));
		}

		T* Acquire() {
			WRITE_FENCE_GUARD();
			size_t cur = currentCount;
			for (size_t i = 0; i < N; i++) {
				T*& head = heads[(i + cur) % N];

				if (head != nullptr) {
					T* p = head;
					head = head->next;
					p->next = nullptr;
					currentCount--;
					return p;
				}
			}

			T* p = allocator.allocate(1);
			allocator.construct(p);
			return p;
		}

		T* AcquireSafe() {
			size_t cur = currentCount;
			std::atomic_thread_fence(std::memory_order_acquire);

			for (size_t i = 0; i < N; i++) {
				T*& head = heads[(i + cur) % N];

				std::atomic<T*>& h = *reinterpret_cast<std::atomic<T*>*>(&head);
				T* p = (T*)h.exchange(nullptr, std::memory_order_relaxed);

				if (p != nullptr) {
					T* next = p->next;
					p->next = nullptr;

					if (next != nullptr) {
						T* t = (T*)h.exchange(next, std::memory_order_release);
						while (t != nullptr) {
							T* q = t;
							t = t->next;
							T* r = (T*)h.load(std::memory_order_relaxed);
							do {
								q->next = r;
							} while (!h.compare_exchange_weak(r, q, std::memory_order_release));
						}
					}

					std::atomic<uint32_t>& count = *reinterpret_cast<std::atomic<uint32_t>*>(&currentCount);
					count.fetch_sub(1, std::memory_order_release);
					return p;
				}
			}

			T* p = allocator.allocate(1);
			allocator.construct(p);
			return p;
		}

		void Release(T* item) {
			WRITE_FENCE_GUARD();
			assert(item->next == nullptr);
			if (currentCount < maxCount) {
				size_t cur = currentCount;
				for (size_t i = 0; i < N; i++) {
					T*& head = heads[(i + cur) % N];
					if (head == nullptr) {
						head = item;
						currentCount++;
						return;
					}
				}

				T*& head = heads[currentCount % N];
				item->next = head;
				head = item;
				currentCount++;
			} else {
				allocator.destroy(item);
				allocator.deallocate(item, 1);
			}
		}

		void ReleaseSafe(T* item) {
			assert(item->next == nullptr);
			std::atomic_thread_fence(std::memory_order_acquire);

			if (currentCount < maxCount) {
				size_t cur = currentCount;
				std::atomic_thread_fence(std::memory_order_acquire);
				std::atomic<uint32_t>& count = *reinterpret_cast<std::atomic<uint32_t>*>(&currentCount);

				for (size_t i = 0; i < N; i++) {
					T*& head = heads[(i + cur) % N];
					if (head == nullptr) {
						std::atomic<T*>& h = *reinterpret_cast<std::atomic<T*>*>(&head);
						T* expected = nullptr;
						if (h.compare_exchange_weak(expected, item, std::memory_order_release, std::memory_order_relaxed)) {
							count.fetch_add(1, std::memory_order_relaxed);
							return;
						}
					}
				}

				T*& head = heads[currentCount % N];
				std::atomic<T*>& h = *reinterpret_cast<std::atomic<T*>*>(&head);
				T* r = h.load(std::memory_order_acquire);
				do {
					item->next = r;
				} while (!h.compare_exchange_weak(r, item, std::memory_order_release, std::memory_order_relaxed));

				count.fetch_add(1, std::memory_order_relaxed);
			} else {
				allocator.destroy(item);
				allocator.deallocate(item, 1);
			}
		}

		void Clear(uint32_t newMaxCount = 0) {
			WRITE_FENCE_GUARD();
			maxCount = 0; // do not recycle anymore
			std::atomic_thread_fence(std::memory_order_release);
			bool loop;

			do {
				loop = false;
				for (size_t i = 0; i < N; i++) {
					T*& head = heads[i % N];
					std::atomic<T*>& h = *reinterpret_cast<std::atomic<T*>*>(&head);
					T* p = (T*)h.exchange(nullptr, std::memory_order_relaxed);
					T* q = p;

					if (p != nullptr) {
						loop = true;

						do {
							p = p->next;
							allocator.destroy(q);
							allocator.deallocate(q, 1);
							q = p;
						} while (p != nullptr);
					}
				}
			} while (loop);

			maxCount = newMaxCount;
			currentCount = 0;
		}

	protected:
		TPool& operator = (const TPool& rhs);
		Allocator& allocator;
		uint32_t maxCount;
		uint32_t currentCount;
		T* heads[N];
	};
}

