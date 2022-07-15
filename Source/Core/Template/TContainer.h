// TBuffer -- Basic buffer structure
// PaintDream (paintdream@paintdream.com)
// 2014-11-30
//

#pragma once
#include "../PaintsNow.h"
#include "../Interface/IType.h"
#include <string>

namespace PaintsNow {
	// Container with some internal storage.
	template <class T, size_t K = 64, size_t S = 64>
	class TContainer {
	public:
		TContainer() : size(0) {
#if defined(_MSC_VER) && _MSC_VER > 1200
			static_assert(std::is_trivially_constructible<T>::value, "Must be trivially constructible.");
			static_assert(std::is_trivially_destructible<T>::value, "Must be trivially destructible.");
#endif
		}

		TContainer(const TContainer& rhs) {
			size = rhs.size;
			memcpy(storage, rhs.storage, sizeof(rhs.storage));

			if (IsExtStorage()) {
				size_t capacity = GetCount() + 1 - N;
				size_t n = (capacity / M + 1) * S;
				external = reinterpret_cast<T*>(malloc(n));
				memcpy(external, rhs.external, n);
			}
		}

		TContainer& operator = (const TContainer& rhs) {
			Clear();

			size = rhs.size;
			memcpy(storage, rhs.storage, sizeof(rhs.storage));

			if (IsExtStorage()) {
				size_t capacity = GetCount() + 1 - N;
				size_t n = (capacity / M + 1) * S;
				external = reinterpret_cast<T*>(malloc(n));
				memcpy(external, rhs.external, n);
			}

			return *this;
		}

		TContainer(rvalue<TContainer> r) {
			TContainer& rhs = r;
			size = rhs.size;
			memcpy(storage, rhs.storage, sizeof(rhs.storage));
			rhs.size = 0;
		}

		TContainer& operator = (rvalue<TContainer> r) {
			Clear();

			TContainer& rhs = r;
			size = rhs.size;
			memcpy(storage, rhs.storage, sizeof(rhs.storage));
			rhs.size = 0;
			return *this;
		}


		~TContainer() {
			Clear();
		}

		enum 
#if !defined(_MSC_VER) || _MSC_VER > 1200
			: size_t
#endif
		{
			N = (K - sizeof(size_t)) / sizeof(T),
			M = S / sizeof(T),
			EXT_STORAGE_MASK = ((size_t)1 << (sizeof(size_t) * 8 - 1))
		};

		void Swap(TContainer& rhs) {
			std::swap(size, rhs.size);
			for (size_t i = 0; i < N; i++) {
				std::swap(storage[i], rhs.storage[i]);
			}
		}

		void Clear() {
			if (IsExtStorage()) {
				free(external);
			}

			size = 0;
		}
		
		bool IsEmpty() const {
			return size == 0;
		}

		void Push(const T& value) {
			static_assert(M > 1, "Object size too large!");

			if (IsExtStorage()) {
				size_t capacity = GetCount() + 1 - N;
				assert(capacity != 0);
				assert(external != nullptr);
				if (capacity % M == 0) {
					external = reinterpret_cast<T*>(realloc(external, (capacity / M + 1) * S));
				}

				external[capacity] = value;
				size++;
			} else {
				size_t count = GetCount();
				if (count == N) {
					T save = storage[N - 1];
					T* p = reinterpret_cast<T*>(malloc(S));
					p[0] = save;
					p[1] = value;
					external = p;

					size = (size + 1) | EXT_STORAGE_MASK;
				} else {
					storage[count] = value;
					size++;
				}
			}
		}

		void Pop() {
			assert(size != 0);
			if (IsExtStorage()) {
				size_t capacity = GetCount() + 1 - N;
				assert(capacity > 1);
				assert(external != nullptr);

				if (capacity == 2) {
					T* p = external;
					storage[N - 1] = external[0];
					free(p);
					size = (size - 1) & ~EXT_STORAGE_MASK;
				} else {
					size--;
				}
			} else {
				size--;
			}
		}

		T& Get(size_t i) {
			if (IsExtStorage()) {
				assert(GetCount() > N);

				if (i + 1 < N) {
					return storage[i];
				} else {
					return external[i + 1 - N];
				}
			} else {
				assert(i < N && i < GetCount());
				return storage[i];
			}
		}

		const T& Get(size_t i) const {
			if (IsExtStorage()) {
				assert(GetCount() > N);

				if (i + 1 < N) {
					return storage[i];
				} else {
					return external[i + 1 - N];
				}
			} else {
				assert(i < N && i < GetCount());
				return storage[i];
			}
		}

		template <class C>
		void Collect(C& container) {
			if (IsExtStorage()) {
				for (size_t i = 0; i < N - 1; i++) {
					container.emplace_back(storage[i]);
				}

				size_t capacity = GetCount() + 1 - N;
				for (size_t n = 0; n < capacity; n++) {
					container.emplace_back(external[n]);
				}
			} else {
				for (size_t i = 0; i < size; i++) {
					container.emplace_back(storage[i]);
				}
			}
		}

		template <class C>
		void CollectExcept(C& container, const T& except) {
			if (IsExtStorage()) {
				for (size_t i = 0; i < N - 1; i++) {
					const T& t = storage[i];
					if (t != except) {
						container.emplace_back(t);
					}
				}

				size_t capacity = GetCount() + 1 - N;
				for (size_t n = 0; n < capacity; n++) {
					const T& t = external[n];
					if (t != except) {
						container.emplace_back(t);
					}
				}
			} else {
				for (size_t i = 0; i < size; i++) {
					const T& t = storage[i];
					if (t != except) {
						container.emplace_back(t);
					}
				}
			}
		}

		T& operator [] (size_t i) {
			return Get(i);
		}

		const T& operator [] (size_t i) const {
			return Get(i);
		}

		bool IsExtStorage() const {
			return !!(size & EXT_STORAGE_MASK);
		}

		size_t GetCount() const {
			return size & ~EXT_STORAGE_MASK;
		}

	protected:
		size_t size;
		union {
			T storage[N];
			struct {
				T part[N - 1];
				T* external;
			};
		};
	};

}

