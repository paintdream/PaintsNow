// TQueue.h
// One to one queue, kfifo.
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../PaintsNow.h"
#include "TAtomic.h"
#include "TAlgorithm.h"
#include "TAllocator.h"
#include <vector>
#include <cassert>

namespace PaintsNow {
	template <size_t i>
	struct clog2 {
		enum { value = i / 2 != 0 ? 1 + clog2<i / 2>::value : 0 };
	};

	template <>
	struct clog2<0> {
		enum { value = 0 };
	};

	// Single-read-single-write
	template <class T, uint32_t Q = 4096, uint32_t C = (64 * 1024 / Q)>
	class TQueue {
	private:
		class Storage {
			uint8_t data[sizeof(T)];
		};

		enum {
			N = Q / sizeof(T)
		};

		enum {
			COUNT_POW_TWO = ((size_t)1 << clog2<N>::value) == N ? 1 : 0,
			COUNTER_LIMIT = (size_t)N * ((size_t)1 << (size_t)(sizeof(size_t) * 8 - 1 - clog2<N>::value)),
		};

	public:
		TQueue(rvalue<TQueue> q) {
			TQueue& queue = q;
			pushCount = queue.pushCount;
			popCount = queue.popCount;
			ringBuffer = queue.ringBuffer;
			next = queue.next;
			queue.ringBuffer = nullptr;
		}

		typedef TRootAllocator<Q, C> Allocator;
		TQueue(uint32_t initCount = 0) : pushCount(initCount), popCount(initCount), next(nullptr) {
			// leave uninitialized
			ringBuffer = new (Allocator::Get().Allocate(UniqueType<TQueue>::Get())) Storage[N];
		}

		struct Destruct {
			bool operator () (T& t) {
				t.~T();
				return true;
			}
		};

		~TQueue() {
			if (ringBuffer != nullptr) {
				Destruct destruct;
				Iterate(destruct);
				Allocator::Get().Deallocate(ringBuffer);
			}
		}

#if defined(_MSC_VER) && _MSC_VER <= 1200
		inline bool Push(const T& t) {
			if (Full()) {
				return false; // full
			}

			new (&ringBuffer[pushCount % N]) T(t);

			std::atomic_thread_fence(std::memory_order_release);
			pushCount = StepCounter(pushCount, 1);

			return true;
		}

		template <class D>
		inline bool Push(rvalue<D> t) {
			if (Full()) {
				return false; // full
			}

			new (&ringBuffer[pushCount % N]) T(t);

			std::atomic_thread_fence(std::memory_order_release);
			pushCount = StepCounter(pushCount, 1);

			return true;
		}
#else
		template <class D>
		inline bool Push(D&& t) {
			if (Full()) {
				return false; // full
			}

			new (&ringBuffer[pushCount % N]) T(std::forward<D>(t));

			std::atomic_thread_fence(std::memory_order_release);
			pushCount = StepCounter(pushCount, 1);

			return true;
		}
#endif
		template <class It>
		It Push(It from, It to) {
			if (Full()) {
				return from;
			}

			It org = from;
			size_t windex = pushCount % N;
			size_t rindex = popCount % N;

			if (rindex <= windex) {
				while (from != to && windex < N) {
					new (&ringBuffer[windex++]) T(*from++);
				}

				windex = 0;
			}

			while (from != to && windex < rindex) {
				new (&ringBuffer[windex++]) T(*from++);
			}

			std::atomic_thread_fence(std::memory_order_release);
			pushCount = StepCounter(pushCount, verify_cast<int32_t>(from - org));

			return from;
		}

		template <class It>
		It Pop(It from, It to) {
			if (Empty()) {
				return from;
			}

			It org = from;
			size_t windex = pushCount % N;
			size_t rindex = popCount % N;

			if (windex <= rindex) {
				while (from != to && rindex < N) {
					T& element = *reinterpret_cast<T*>(&ringBuffer[rindex++]);
#if !defined(_MSC_VER) || _MSC_VER > 1200
					*from++ = std::move(element);
#else
					*from++ = element;
#endif
					element.~T();
				}

				rindex = 0;
			}

			while (from != to && rindex < windex) {
				T& element = *reinterpret_cast<T*>(&ringBuffer[rindex++]);
#if !defined(_MSC_VER) || _MSC_VER > 1200
				*from++ = std::move(element);
#else
				*from++ = element;
#endif
				element.~T();
			}

			std::atomic_thread_fence(std::memory_order_acquire);
			popCount = StepCounter(popCount, verify_cast<int32_t>(from - org));

			return from;
		}

		inline void Pop() {
			reinterpret_cast<T*>(&ringBuffer[popCount % N])->~T();
			std::atomic_thread_fence(std::memory_order_acquire);
			popCount = StepCounter(popCount, 1);
		}

		uint32_t Pop(uint32_t n) {
			uint32_t m = Math::Min(n, Count());
			for (size_t i = 0; i < m; i++) {
				reinterpret_cast<T*>(&ringBuffer[(popCount + i) % N])->~T();
			}

			size_t rindex = popCount % N;
			size_t k = m;
			while (rindex < N && k != 0) {
				reinterpret_cast<T*>(&ringBuffer[rindex++])->~T();
				k--;
			}

			if (k != 0) {
				rindex = 0;
				do {
					reinterpret_cast<T*>(&ringBuffer[rindex++])->~T();
				} while (--k != 0);
			}

			std::atomic_thread_fence(std::memory_order_acquire);

			popCount = StepCounter(popCount, m);
			return n - m;
		}

		// for prefetch
		inline const T& Predict() const {
			return *reinterpret_cast<const T*>(&ringBuffer[(StepCounter(popCount, 1)) % N]);
		}

		inline T* Allocate(uint32_t count, uint32_t alignment) {
			assert(count >= alignment);
			assert(count <= N);
			// Make alignment
			uint32_t pushIndex = pushCount % N;
			uint32_t padding = (uint32_t)(alignment - Math::Alignment(pushIndex)) & (alignment - 1);
			count += padding;
			if (count > N - Count()) return nullptr;

			uint32_t nextIndex = pushIndex + count;
			if (count != 1 && nextIndex > N) return nullptr; // non-continous!

			uint32_t retIndex = pushIndex + padding;
			for (uint32_t i = pushIndex; i != nextIndex; i++) {
				new (&ringBuffer[i]) T; // i % N not needed
			}

			std::atomic_thread_fence(std::memory_order_release);
			pushCount = StepCounter(pushCount, count);

			return reinterpret_cast<T*>(ringBuffer + retIndex);
		}

		inline bool Deallocate(uint32_t count, uint32_t alignment) {
			assert(count >= alignment);
			assert(count <= N);

			// Make alignment
			uint32_t popIndex = popCount % N;
			count += (uint32_t)(alignment - Math::Alignment(popIndex)) & (alignment - 1);
			if (count > Count())
				return false;

			uint32_t nextIndex = popIndex + count;
			assert(nextIndex <= N);

			for (uint32_t i = popIndex; i != nextIndex; i++) {
				reinterpret_cast<T*>(&ringBuffer[i])->~T();
			}

			std::atomic_thread_fence(std::memory_order_release);
			popCount = StepCounter(popCount, count);

			return true;
		}

		inline void Clear() {
			if (ringBuffer != nullptr) {
				Destruct destruct;
				Iterate(destruct);
			}

			popCount = pushCount;
		}

		inline void Reset(uint32_t initCount) {
			if (ringBuffer != nullptr) {
				Destruct destruct;
				Iterate(destruct);
			}

			std::atomic_thread_fence(std::memory_order_release);
			pushCount = popCount = initCount;
		}

		inline T& Top() {
			assert(!Empty());
			return *reinterpret_cast<T*>(&ringBuffer[popCount % N]);
		}

		inline const T& Top() const {
			assert(!Empty());
			return *reinterpret_cast<const T*>(&ringBuffer[popCount % N]);
		}

		template <class F>
#if defined(_MSC_VER) && _MSC_VER <= 1200
		inline bool Iterate(F& f) {
#else
		inline bool Iterate(F&& f) {
#endif
			if (Empty())
				return true;

			size_t windex = pushCount % N;
			size_t rindex = popCount % N;

			if (rindex >= windex) {
				while (rindex < N) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
					if (!f(*reinterpret_cast<T*>(&ringBuffer[rindex++]))) {
#else
					if (!(std::forward<F>(f))(*reinterpret_cast<T*>(&ringBuffer[rindex++]))) {
#endif
						return false;
					}
				}

				rindex = 0;
			}

			while (rindex < windex) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
				if (!f(*reinterpret_cast<T*>(&ringBuffer[rindex++]))) {
#else
				if (!(std::forward<F>(f))(*reinterpret_cast<T*>(&ringBuffer[rindex++]))) {
#endif
					return false;
				}
			}

			return true;
		}

		inline bool Empty() const {
			return popCount == pushCount;
		}

		inline bool Full() const {
			return Count() == N;
		}
		
		inline uint32_t Count() const {
			return (uint32_t)DiffCounter(pushCount, popCount);
		}

		inline uint32_t GetPackCount(uint32_t alignment) const {
			assert(N >= alignment);
			uint32_t index = pushCount + (uint32_t)(alignment - Math::Alignment(pushCount % N)) & (alignment - 1);
			return Math::Min(Math::Max(index, popCount + N) - index, N - (index % N));
		}

		inline uint32_t Begin() const {
			return popCount;
		}

		inline uint32_t End() const {
			return pushCount;
		}

		inline uint32_t begin() const {
			return Begin();
		}

		inline uint32_t end() const {
			return End();
		}

		inline T& Get(uint32_t index) {
			return *reinterpret_cast<T*>(&ringBuffer[index % N]);
		}

		inline const T& Get(uint32_t index) const {
			return *reinterpret_cast<T*>(&ringBuffer[index % N]);
		}

		static uint32_t GetFullPackCount() {
			return N;
		}

	public:
		static uint32_t StepCounter(uint32_t count, int32_t delta) {
			uint32_t result = count + delta;
			if /* constexpr */ (!COUNT_POW_TWO) {
				result = result >= COUNTER_LIMIT ? result - COUNTER_LIMIT : result; // cmov is faster than mod
			}

			return result;
		}

		static int32_t DiffCounter(uint32_t lhs, uint32_t rhs) {
			if /* constexpr */ (COUNT_POW_TWO) {
				return (int32_t)lhs - (int32_t)rhs;
			} else {
				int32_t diff = (int32_t)(lhs + COUNTER_LIMIT - rhs);
				return diff >= COUNTER_LIMIT ? diff - COUNTER_LIMIT : diff;  // cmov is faster than mod
			}
		}

	protected:
		uint32_t pushCount;
		uint32_t popCount;
		Storage* ringBuffer;

	public:
		TQueue* next;
	};

	// Still single-read-single-write
	template <class T, size_t Q = 4096, size_t C = (64 * 1024 / Q)>
	class TQueueList : protected TEnableInOutFence<> {
	public:
		typedef TQueue<T, Q, C> Node;
		enum { N = Q / sizeof(T) };

	protected:
		Node* pushHead;
		Node* popHead; // popHead is always prior to pushHead
		uint32_t nodeCount;

	public:
		TQueueList() : nodeCount(Node::GetFullPackCount()) {
			pushHead = popHead = new Node();
		}

		TQueueList(const TQueueList& rhs) : nodeCount(Node::GetFullPackCount()) {
			assert(rhs.Empty());
			pushHead = popHead = new Node();
		}

		TQueueList(rvalue<TQueueList> rhs) {
			pushHead = rhs.pushHead;
			popHead = rhs.popHead;
			nodeCount = rhs.nodeCount;

			rhs.pushHead = nullptr;
			rhs.popHead = nullptr;
		}

		TQueueList& operator = (const TQueueList& rhs) {
			assert(rhs.Empty());
			Clear();
			return *this;
		}

		TQueueList& operator = (rvalue<TQueueList> r) {
			TQueueList& rhs = r;
			IN_FENCE_GUARD();
			OUT_FENCE_GUARD();

			std::swap(pushHead, rhs.pushHead);
			std::swap(popHead, rhs.popHead);
			std::swap(nodeCount, rhs.nodeCount);

			return *this;
		}

		// not a thread safe destructor.
		~TQueueList() {
			IN_FENCE_GUARD();
			OUT_FENCE_GUARD();

			if (popHead != nullptr) {
				// free memory
				Node* q = popHead;
				while (q != nullptr) {
					Node* p = q;
					q = q->next;
					delete p;
				}
			}
		}

		void Clear() {
			IN_FENCE_GUARD();
			OUT_FENCE_GUARD();

			while (popHead != pushHead) {
				Node* q = popHead->next;
				delete popHead;

				popHead = q;
			}

			popHead->Clear();
		}

		void Preserve() {
			if (pushHead->Full()) {
				Node* p = new Node(nodeCount);
				nodeCount = Node::StepCounter(nodeCount, Node::GetFullPackCount());

				// chain new node_t at head.
				pushHead->next = p;
				pushHead = p;
			}
		}

#if defined(_MSC_VER) && _MSC_VER <= 1200
		template <class D>
		inline void Push(D& t) {
			IN_FENCE_GUARD();

			Preserve();
			pushHead->Push(t);
		}
#else
		template <class D>
		inline void Push(D&& t) {
			IN_FENCE_GUARD();

			Preserve();
			pushHead->Push(std::forward<D>(t));
		}
#endif

		template <class It>
		It Push(It from, It to) {
			IN_FENCE_GUARD();

			while (true) {
				It next = pushHead->Push(from, to);
				if (from == next)
					return next; // complete

				from = next;
				Preserve();
			}
		}

		template <class It>
		It Pop(It from, It to) {
			OUT_FENCE_GUARD();

			while (true) {
				It next = popHead->Pop(from, to);
				if (from == next) {
					if (popHead->Empty() && popHead != pushHead) {
						Node* p = popHead;
						popHead = popHead->next;

						delete p;
					}

					if (next == to)
						return next;
				}

				from = next;
			}
		}

		inline T& Top() {
			return popHead->Top();
		}

		inline const T& Top() const {
			return popHead->Top();
		}

		inline const T& Predict() const {
			return popHead->Predict();
		}

		inline void Pop() {
			OUT_FENCE_GUARD();
			popHead->Pop();

			if (popHead->Empty() && popHead != pushHead) {
				Node* p = popHead;
				popHead = popHead->next;

				delete p;
			}
		}

		inline uint32_t Pop(uint32_t n) {
			OUT_FENCE_GUARD();

			while (n != 0) {
				uint32_t m = Math::Min(n, popHead->Count());
				popHead->Pop(m);

				// current queue is empty, remove it from list.
				if (popHead->Empty() && popHead != pushHead) {
					Node* p = popHead;
					popHead = popHead->next;
					delete p;

					n -= m;
				} else {
					break;
				}
			}

			return n;
		}

		inline uint32_t GetPackCount(uint32_t alignment) const {
			uint32_t v = pushHead->GetPackCount(alignment);
			return v == 0 ? GetFullPackCount() : v;
		}

		inline T* Allocate(uint32_t count, uint32_t alignment) {
			IN_FENCE_GUARD();

			T* address;
			while ((address = pushHead->Allocate(count, alignment)) == nullptr) {
				if (pushHead->next == nullptr) {
					Node* p = new Node(nodeCount);
					nodeCount = Node::StepCounter(nodeCount, Node::GetFullPackCount());

					address = p->Allocate(count, alignment); // Must success
					assert(address != nullptr);

					pushHead->next = p;
					pushHead = p;
					return address;
				}

				pushHead = pushHead->next;
			}

			return address;
		}

		inline void Deallocate(uint32_t size, uint32_t alignment) {
			OUT_FENCE_GUARD();

			while (!popHead->Deallocate(size, alignment)) {
				if (popHead != pushHead) {
					Node* p = popHead;
					popHead = popHead->next;
					delete p;
				} else {
					assert(false);
					break;
				}
			}
		}

		inline void Reset(uint32_t reserved) {
			IN_FENCE_GUARD();
			OUT_FENCE_GUARD();

			Node* p = pushHead = popHead;
			p->Reset(0); // always reserved
			nodeCount = GetFullPackCount();
			Node* q = p;
			p = p->next;

			while (p != nullptr && nodeCount < reserved) {
				p->Reset(nodeCount);
				nodeCount = Node::StepCounter(nodeCount, Node::GetFullPackCount());
				q = p;
				p = p->next;
			}

			while (p != nullptr) {
				Node* t = p;
				p = p->next;
				delete t;
			}

			q->next = nullptr;
		}

		inline bool Empty() const {
			return popHead->Empty();
		}

		class Iterator {
		public:
			typedef uint32_t difference_type;
			typedef T value_type;
			typedef T& reference;
			typedef T* pointer;
			typedef std::forward_iterator_tag iterator_category;

			// just for trivially construct
			Iterator() {}
			Iterator(Node* n, uint32_t t) : p(n), it(t) {}
			Iterator& operator ++ () {
				it = Node::StepCounter(it, 1);
				if (it == p->End()) {
					Node* t = p->next;
					if (t != nullptr) {
						p = t;
						it = p->Begin();
					}
				}

				return *this;
			}

			Iterator& operator = (const Iterator& rhs) {
				p = rhs.p;
				it = rhs.it;
				return *this;
			}

			Iterator& operator += (uint32_t count) {
				Iterator p = *this + count;
				*this = std::move(p);
				return *this;
			}

			Iterator operator + (uint32_t count) const {
				Node* n = p;
				uint32_t sub = it;
				while (true) {
					uint32_t c = (uint32_t)Node::DiffCounter(n->End(), sub);
					if (count >= c) {
						count -= c;
						Node* t = n->next;
						if (t == nullptr) {
							return Iterator(n, n->End());
						}

						n = t;
						sub = t->Begin();
					} else {
						return Iterator(n, Node::StepCounter(n->Begin(), count));
					}
				}
			}

			uint32_t operator - (const Iterator& rhs) const {
				Node* t = rhs.p;
				uint32_t sub = rhs.it;
				uint32_t count = 0;

				while (t != p) {
					count += (uint32_t)Node::DiffCounter(t->End(), sub);
					t = t->next;
					sub = t->Begin();
				}

				count += (uint32_t)Node::DiffCounter(it, sub);
				return count;
			}


			bool operator == (const Iterator& t) const {
				return /* p == t.p && */ it == t.it;
			}

			bool operator != (const Iterator& t) const {
				return /* p != t.p || */ it != t.it;
			}

			T& operator * () const {
				return p->Get(it);
			}

			T* operator -> () const {
				return &p->Get(it);
			}


			uint32_t it;
			Node* p;
		};

		typedef Iterator iterator;

		inline Iterator Begin() {
			return Iterator(popHead, popHead->Begin());
		}

		inline Iterator End() {
			return Iterator(pushHead, pushHead->End());
		}

		inline Iterator begin() {
			return Begin();
		}

		inline Iterator end() {
			return End();
		}

		class ConstIterator {
		public:
			typedef uint32_t difference_type;
			typedef const T value_type;
			typedef const T& reference;
			typedef const T* pointer;
			typedef std::forward_iterator_tag iterator_category;

			// just for trivially construct
			ConstIterator() {}
			ConstIterator(const Node* n, uint32_t t) : p(n), it(t) {}
			ConstIterator(const Iterator& rhs) : p(rhs.p), it(rhs.it) {}
			ConstIterator& operator ++ () {
				it = Node::StepCounter(it, 1);
				if (it == p->End()) {
					Node* t = p->next;
					if (t != nullptr) {
						p = t;
						it = p->Begin();
					}
				}

				return *this;
			}

			ConstIterator& operator = (const ConstIterator& rhs) {
				p = rhs.p;
				it = rhs.it;
				return *this;
			}

			ConstIterator& operator += (uint32_t count) {
				ConstIterator p = *this + count;
				*this = std::move(p);
				return *this;
			}

			ConstIterator operator + (uint32_t count) const {
				const Node* n = p;
				size_t sub = it;
				while (true) {
					uint32_t c = (uint32_t)Node::DiffCounter(n->End(), sub);
					if (count >= c) {
						count -= c;
						const Node* t = n->next;
						if (t == nullptr) {
							return ConstIterator(n, n->End());
						}

						n = t;
						sub = t->Begin();
					} else {
						return ConstIterator(n, Node::StepCounter(n->Begin(), count));
					}
				}
			}

			uint32_t operator - (const ConstIterator& rhs) const {
				const Node* t = rhs.p;
				uint32_t sub = rhs.it;
				uint32_t count = 0;

				while (t != p) {
					count += (uint32_t)Node::DiffCounter(t->End(), sub);
					t = t->next;
					sub = t->Begin();
				}

				count += (uint32_t)Node::DiffCounter(it, sub);
				return count;
			}


			bool operator == (const ConstIterator& t) const {
				return /* p == t.p && */ it == t.it;
			}

			bool operator != (const ConstIterator& t) const {
				return /* p != t.p || */ it != t.it;
			}

			const T& operator * () const {
				return p->Get(it);
			}

			const T* operator -> () const {
				return &p->Get(it);
			}

			uint32_t it;
			const Node* p;
		};

		typedef ConstIterator const_iterator;

		inline ConstIterator Begin() const {
			return ConstIterator(popHead, popHead->Begin());
		}

		inline ConstIterator End() const {
			return ConstIterator(pushHead, pushHead->End());
		}

		inline ConstIterator begin() const {
			return Begin();
		}

		inline ConstIterator end() const {
			return End();
		}

		inline uint32_t Count() const {
			uint32_t counter = 0;
			for (Node* p = popHead; p != nullptr; p = p->next) {
				counter += p->Count();
			}

			return counter;
		}

		static inline uint32_t GetFullPackCount() {
			return Node::GetFullPackCount();
		}

		template <class F>
#if defined(_MSC_VER) && _MSC_VER <= 1200
		inline bool Iterate(F& f) {
#else
		inline bool Iterate(F&& f) {
#endif
			OUT_FENCE_GUARD();

			for (Node* p = popHead; p != nullptr; p = p->next) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
				if (!p->Iterate(f)) {
#else
				if (!p->Iterate(std::forward<F>(f))) {
#endif
					return false;
				}
			}

			return true;
		}
	};

	template <class T, class F = TQueueList<typename T::Iterator> >
	class TQueueFrame {
	public:
		TQueueFrame(T& q) : queue(q), barrier(q.End()) {}

		typedef typename T::Iterator Iterator;
		typedef typename T::iterator iterator;
		typedef typename T::ConstIterator ConstIterator;
		typedef typename T::const_iterator const_iterator;

		Iterator Begin() {
			return queue.Begin();
		}

		Iterator End() {
			return barrier;
		}

		Iterator begin() {
			return Begin();
		}

		Iterator end() {
			return barrier;
		}

		ConstIterator Begin() const {
			return queue.Begin();
		}

		ConstIterator End() const {
			return barrier;
		}

		ConstIterator begin() const {
			return Begin();
		}

		ConstIterator end() const {
			return barrier;
		}

		template <class D>
#if defined(_MSC_VER) && _MSC_VER <= 1200
		void Push(D& element) {
			queue.Push(element);
		}
#else
		void Push(D&& element) {
			queue.Push(std::forward<D>(element));
		}
#endif
		template <class It>
		It Push(It from, It to) {
			return queue.Push(from, to);
		}

		template <class It>
		It Pop(It from, It to) {
			return queue.Pop(from, to);
		}

		uint32_t Count() const {
			return end() - begin();
		}

		bool Acquire() {
			if (!queue.Empty()) {
				queue.Pop(barrier - Begin());
			}

			if (!frames.Empty()) {
				barrier = frames.Top();
				frames.Pop();
				return true;
			} else {
				return false;
			}
		}

		bool AcquireCatchup(uint32_t remain = 0) {
			while (frames.Count() > remain) {
				Acquire();
			}

			return frames.Count() >= remain;
		}

		void Release() {
			frames.Push(queue.End());
		}

		void Clear() {
			while (Acquire()) {}
		}

	protected:
		T& queue;
		Iterator barrier;
		F frames;
	};
}

