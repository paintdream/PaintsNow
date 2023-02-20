// TAllocator.h
// PaintDream (paintdream@paintdream.com)
// 2019-10-9
//

#pragma once
#include "TAtomic.h"
#include "TAlgorithm.h"
#include "../Interface/IMemory.h"
#include "../System/Tiny.h"
#include <utility>

namespace PaintsNow {
	// Global allocator that allocates memory blocks to local allocators.
	template <size_t N, size_t K>
	class TRootAllocator {
	public:
		TRootAllocator() { critical.store(0, std::memory_order_relaxed); }
		~TRootAllocator() {
			pinObjects.clear();
			assert(blocks.empty());
		}

		enum { BITMAP_COUNT = (K + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8) };

		void* Allocate(Unique typeTag) {
			// do fast operations in critical section
			do {
				TSpinLockGuard<size_t> guard(critical);
				for (size_t i = 0; i < blocks.size(); i++) {
					Block& block = blocks[i];
					for (size_t k = 0; k < BITMAP_COUNT; k++) {
						size_t& bitmap = block.bitmap[k];
						size_t bit = bitmap + 1;
						bit = bit & (~bit + 1);
						if (bit != 0) {
							size_t index = Math::Log2x(bit) + k * sizeof(size_t) * 8;
							if (index < K) {
								bitmap |= bit;
								return block.address + index * N;
							}
						}
					}
				}
			} while (false);

			// real allocation, release the critical.
			Block block;
			block.address = reinterpret_cast<uint8_t*>(IMemory::AllocAligned(N * K, N));
			memset(block.bitmap, 0, sizeof(block.bitmap));
			block.bitmap[0] = 1;
			block.tag = typeTag;

			// write result back
			do {
				TSpinLockGuard<size_t> guard(critical);
				blocks.emplace_back(block);
			} while (false);

			return block.address;
		}

		void Deallocate(void* p) {
			void* t = nullptr;

			do {
				TSpinLockGuard<size_t> guard(critical);

				// loop to find required one.
				for (size_t i = 0; i < blocks.size(); i++) {
					Block& block = blocks[i];
					if (p >= block.address && p < block.address + N * K) {
						size_t index = (reinterpret_cast<uint8_t*>(p) - block.address) / N;
						size_t page = index / (sizeof(size_t) * 8);
						size_t offset = index & (sizeof(size_t) * 8 - 1);
						assert(page < BITMAP_COUNT);
						size_t& bitmap = block.bitmap[page];
						bitmap &= ~((size_t)1 << offset);

						if (bitmap == 0) {
							size_t k;
							for (k = 0; k < BITMAP_COUNT; k++) {
								if (block.bitmap[k] != 0)
									break;
							}

							if (k == BITMAP_COUNT) {
								t = block.address;
								blocks.erase(blocks.begin() + i);
							}
						}

						break;
					}
				}
			} while (false);

			if (t != nullptr) {
				// do free
				IMemory::FreeAligned(t, N * K);
			}
		}

		// We are not dll-friendly, as always.
		static TRootAllocator& Get() {
			return TSingleton<TRootAllocator>::Get();
		}

		// Pin global static objects that must be destructed before me
		void Pin(const TShared<SharedTiny>& rhs) {
			TSpinLockGuard<size_t> guard(critical);
			pinObjects.emplace_back(rhs);
		}

	protected:
		struct Block {
			uint8_t* address;
			Unique tag;
			size_t bitmap[BITMAP_COUNT];
		};

	protected:
		std::atomic<size_t> critical;
		std::vector<Block> blocks;
		std::vector<TShared<SharedTiny> > pinObjects;
	};

	// Local allocator, allocate memory with specified alignment requirements.
	// K = element size, M = block size, R = max recycled block count, 0 for not limited, W = head cache count
	template <size_t K, size_t M = 4096, size_t R = 8, size_t S = 16, size_t W = 4>
	class TAllocator : public TReflected<TAllocator<K, M, R>, SharedTiny> {
	public:
		typedef TReflected<TAllocator<K, M, R>, SharedTiny> BaseClass;
		enum {
			SIZE = M,
			N = M / K,
			BITS = 8 * sizeof(size_t),
			BITMAPSIZE = (N + BITS - 1) / BITS,
			MASK = BITS - 1
		};

		class ControlBlock {
		public:
			TAllocator* allocator;
			ControlBlock* next;
			std::atomic<uint32_t> refCount;
			std::atomic<uint32_t> managed;
			std::atomic<size_t> bitmap[BITMAPSIZE];
		};

		enum {
			OFFSET = (sizeof(ControlBlock) + K - 1) / K
		};

	public:
		TAllocator() {
			static_assert(N / 2 * K > sizeof(ControlBlock), "N is too small");
			recycleCount.store(0, std::memory_order_relaxed);
			for (size_t n = 0; n < sizeof(controlBlocks) / sizeof(controlBlocks[0]); n++) {
				controlBlocks[n].store(nullptr, std::memory_order_relaxed);
			}

			recycleHead.store(nullptr, std::memory_order_release);
		}

		static TRootAllocator<M, S>& GetRootAllocator() {
			return TRootAllocator<M, S>::Get();
		}

		~TAllocator() override {
			// deallocate all caches
			TRootAllocator<M, S>& allocator = GetRootAllocator();

			for (size_t n = 0; n < sizeof(controlBlocks) / sizeof(controlBlocks[0]); n++) {
				ControlBlock* p = (ControlBlock*)controlBlocks[n].load(std::memory_order_acquire);
				if (p != nullptr) {
					allocator.Deallocate(p);
				}
			}

			ControlBlock* p = recycleHead.load(std::memory_order_acquire);
			while (p != nullptr) {
				ControlBlock* t = p->next;
				allocator.Deallocate(p);
				p = t;
			}
		}

		inline void* Allocate() {
			while (true) {
				ControlBlock* p = nullptr;
				for (size_t n = 0; p == nullptr && n < sizeof(controlBlocks) / sizeof(controlBlocks[0]); n++) {
					p = controlBlocks[n].exchange(nullptr, std::memory_order_acquire);
				}

				if (p == nullptr) {
					// need a new block
					p = recycleHead.exchange(nullptr, std::memory_order_acquire);
					if (p != nullptr) {
						ControlBlock* t = p->next;
						ControlBlock* expected = nullptr;
						if (!recycleHead.compare_exchange_strong(expected, t, std::memory_order_release, std::memory_order_relaxed)) {
							while (t != nullptr) {
								ControlBlock* q = t->next;
								ControlBlock* h = recycleHead.load(std::memory_order_relaxed);
								do {
									t->next = h;
								} while (!recycleHead.compare_exchange_weak(h, t, std::memory_order_release, std::memory_order_relaxed));

								t = q;
							}
						}

						p->next = nullptr;
						assert(p->refCount.load(std::memory_order_acquire) >= 1);
						recycleCount.fetch_sub(1, std::memory_order_relaxed);
						assert(p->managed.load(std::memory_order_acquire) == 1);
						p->managed.store(0, std::memory_order_release);
					} else {
						p = reinterpret_cast<ControlBlock*>(GetRootAllocator().Allocate(UniqueType<TAllocator>::Get()));
						memset(p, 0, sizeof(ControlBlock));
						p->next = nullptr;
						p->allocator = this;
						p->refCount.store(1, std::memory_order_relaxed); // newly allocated one, just set it to 1
					}
				} else {
					p->managed.store(0, std::memory_order_release);
				}

				// search for an empty slot
				for (size_t k = 0; k < BITMAPSIZE; k++) {
					std::atomic<size_t>& s = p->bitmap[k];
					size_t mask = s.load(std::memory_order_acquire);
					if (mask != ~(size_t)0) {
						size_t bit = Math::Alignment(mask + 1);
						if (!(s.fetch_or(bit, std::memory_order_relaxed) & bit)) {
							// get index of bitmap
							size_t index = Math::Log2x(bit) + OFFSET + k * 8 * sizeof(size_t);
							if (index < N) {
								p->refCount.fetch_add(1, std::memory_order_relaxed);

								BaseClass::ReferenceObject();
								// add to recycle system if needed
								Recycle(p);

								return reinterpret_cast<char*>(p) + index * K;
							}
						}
					}
				}

				// full?
				TryFree(p);
			}

			assert(false);
			return nullptr; // never reach here
		}

		static inline void Deallocate(void* ptr) {
			size_t t = reinterpret_cast<size_t>(ptr);
			ControlBlock* p = reinterpret_cast<ControlBlock*>(t & ~(SIZE - 1));
			size_t id = (t - (size_t)p) / K - OFFSET;
			p->allocator->Deallocate(p, id);
		}

	protected:
		inline void TryFree(ControlBlock* p) {
			assert(p->refCount.load(std::memory_order_acquire) != 0);
			if (p->refCount.fetch_sub(1, std::memory_order_release) == 1) {
				for (size_t n = 0; n < sizeof(controlBlocks) / sizeof(controlBlocks[0]); n++) {
					assert(controlBlocks[n].load(std::memory_order_acquire) != p);
				}

				GetRootAllocator().Deallocate(p);
			}
		}

		inline void Recycle(ControlBlock* p) {
			assert(p->refCount.load(std::memory_order_acquire) != 0);
			// search for recycled
			if (p->managed.load(std::memory_order_acquire) == 0
				&& recycleCount.load(std::memory_order_relaxed) < R
				&& p->managed.exchange(1, std::memory_order_acquire) == 0) {
				for (size_t n = 0; n < sizeof(controlBlocks) / sizeof(controlBlocks[0]); n++) {
					ControlBlock* expected = nullptr;
					if (controlBlocks[n].compare_exchange_weak(expected, p, std::memory_order_release, std::memory_order_relaxed)) {
						return;
					}
				}

				recycleCount.fetch_add(1, std::memory_order_relaxed);

				assert(p->next == nullptr);
				ControlBlock* h = recycleHead.load(std::memory_order_relaxed);
				do {
					p->next = h;
				} while (!recycleHead.compare_exchange_weak(h, p, std::memory_order_release, std::memory_order_relaxed));
			} else {
				TryFree(p);
			}
		}

		inline void Deallocate(ControlBlock* p, size_t id) {
			assert(p->allocator != nullptr);
			p->bitmap[id / BITS].fetch_and(~((size_t)1 << (id & MASK)));

			Recycle(p);
			BaseClass::ReleaseObject();
		}

	protected:
		std::atomic<ControlBlock*> controlBlocks[W];
		std::atomic<ControlBlock*> recycleHead;
		std::atomic<size_t> recycleCount;
	};

	// Allocate for objects
	template <class T, size_t M = 4096, size_t Align = 64, size_t R = 8>
	class TObjectAllocator : protected TAllocator<(sizeof(T) + Align - 1) & ~(Align - 1), M, R> {
	public:
		typedef TAllocator<(sizeof(T) + Align - 1) & ~(Align - 1), M, R> Base;
		static inline void Delete(T* object) {
			object->~T();
			Base::Deallocate(object);
		}

		forceinline void ReferenceObject() {
			Base::ReferenceObject();
		}

		forceinline void ReleaseObject() {
			Base::ReleaseObject();
		}

		inline T* allocate(size_t n) {
			assert(n == 1);
			return reinterpret_cast<T*>(Base::Allocate());
		}

#if defined(_MSC_VER) && _MSC_VER <= 1200
		inline T* New() {
			void* ptr = Allocate();
			return new (ptr) T();
		}

		inline void construct(T* p) {
			new (p) T();
		}

		template <class A>
		inline T* New(A a) {
			void* ptr = Allocate();
			return new (ptr) T(a);
		}

		template <class A>
		inline void construct(T* p, A a) {
			new (p) T(a);
		}

		template <class A, class B>
		inline T* New(A a, B b) {
			void* ptr = Allocate();
			return new (ptr) T(a, b);
		}

		template <class A, class B>
		inline void construct(T* p, A a, B b) {
			new (p) T(a, b);
		}

		template <class A, class B, class C>
		inline T* New(A a, B b, C c) {
			void* ptr = Allocate();
			return new (ptr) T(a, b, c);
		}

		template <class A, class B, class C>
		inline void construct(T* p, A a, B b, C c) {
			new (p) T(a, b, c);
		}

		template <class A, class B, class C, class D>
		inline T* New(A a, B b, C c, D d) {
			void* ptr = Allocate();
			return new (ptr) T(a, b, c, d);
		}

		template <class A, class B, class C, class D>
		inline void construct(T* p, A a, B b, C c, D d) {
			new (p) T(a, b, c, d);
		}

		template <class A, class B, class C, class D, class E>
		inline T* New(A a, B b, C c, D d, E e) {
			void* ptr = Allocate();
			return new (ptr) T(a, b, c, d, e);
		}

		template <class A, class B, class C, class D, class E>
		inline void construct(T* p, A a, B b, C c, D d, E e) {
			new (p) T(a, b, c, d, e);
		}

		template <class A, class B, class C, class D, class E, class F>
		inline T* New(A a, B b, C c, D d, E e, F f) {
			void* ptr = Allocate();
			return new (ptr) T(a, b, c, d, e, f);
		}

		template <class A, class B, class C, class D, class E, class F>
		inline void construct(T* p, A a, B b, C c, D d, E e, F f) {
			new (p) T(a, b, c, d, e, f);
		}

		template <class A, class B, class C, class D, class E, class F, class G>
		inline T* New(A a, B b, C c, D d, E e, F f, G g) {
			void* ptr = Allocate();
			return new (ptr) T(a, b, c, d, e, f, g);
		}

		template <class A, class B, class C, class D, class E, class F, class G>
		inline void construct(T* p, A a, B b, C c, D d, E e, F f, G g) {
			new (p) T(a, b, c, d, e, f, g);
		}

		template <class A, class B, class C, class D, class E, class F, class G, class H>
		inline T* New(A a, B b, C c, D d, E e, F f, G g, H h) {
			void* ptr = Allocate();
			return new (ptr) T(a, b, c, d, e, f, g, h);
		}

		template <class A, class B, class C, class D, class E, class F, class G, class H>
		inline void construct(T* p, A a, B b, C c, D d, E e, F f, G g, H h) {
			new (p) T(a, b, c, d, e, f, g, h);
		}
#else
		template <typename... Args>
		inline T* New(Args&&... args) {
			void* ptr = Base::Allocate();
			return new (ptr) T(std::forward<Args>(args)...);
		}

		template <typename... Args>
		inline void construct(T* p, Args&&... args) {
			new (p) T(std::forward<Args>(args)...);
		}
#endif
		inline void destroy(T* p) {
			p->~T();
		}

		inline void deallocate(T* p, size_t n) {
			assert(n == 1);
			Base::Deallocate(p);
		}
	};

	template <class T, class B, size_t M = 4096, size_t Align = 64>
	class pure_interface TAllocatedTiny : public TReflected<T, B> {
	public:
		typedef TObjectAllocator<T, M, Align> Allocator;
		typedef TAllocatedTiny BaseClass;
#if defined(_MSC_VER) && _MSC_VER <= 1200
		TAllocatedTiny() {}
		template <class A>
		TAllocatedTiny(A& a) : TReflected<T, B>(a) {}
		template <class A, class B>
		TAllocatedTiny(A& a, B& b) : TReflected<T, B>(a, b) {}
		template <class A, class B, class C>
		TAllocatedTiny(A& a, B& b, C& c) : TReflected<T, B>(a, b, c) {}
		template <class A, class B, class C, class D>
		TAllocatedTiny(A& a, B& b, C& c, D& d) : TReflected<T, B>(a, b, c, d) {}
		template <class A, class B, class C, class D, class E>
		TAllocatedTiny(A& a, B& b, C& c, D& d, E& e) : TReflected<T, B>(a, b, c, d, e) {}
		template <class A, class B, class C, class D, class E, class F>
		TAllocatedTiny(A& a, B& b, C& c, D& d, E& e, F& f) : TReflected<T, B>(a, b, c, d, e, f) {}
		template <class A, class B, class C, class D, class E, class F, class G>
		TAllocatedTiny(A& a, B& b, C& c, D& d, E& e, F& f, G& g) : TReflected<T, B>(a, b, c, d, e, f, g) {}
		template <class A, class B, class C, class D, class E, class F, class G, class H>
		TAllocatedTiny(A& a, B& b, C& c, D& d, E& e, F& f, G& g, H& h) : TReflected<T, B>(a, b, c, d, e, f, g, h) {}
#else
		template <typename... Args>
		TAllocatedTiny(Args&&... args) : TReflected<T, B>(std::forward<Args>(args)...) {}
#endif

		void Destroy() override {
			Allocator::Delete(static_cast<T*>(this));
		}
	};
}
