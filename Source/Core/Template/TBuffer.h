// TBuffer -- Basic buffer structure
// PaintDream (paintdream@paintdream.com)
// 2014-11-30
//

#pragma once
#include "../PaintsNow.h"
#include "../Interface/IType.h"
#include <string>

namespace PaintsNow {
	// Memory buffer with some internal storage.
	template <class T, size_t N = (sizeof(T*) * 4 - sizeof(size_t)) / sizeof(T)>
	class TBuffer {
	public:
		// union masks with class member `size` for less memory usage
		enum
#if !defined(_MSC_VER) || _MSC_VER > 1200
			: size_t
#endif
		{
			// EXT_STORE_MASK: data stored in extra storage, pointed by `buffer`
			EXT_STORE_MASK = ((size_t)1 << (sizeof(size_t) * 8 - 1)),
			// DATA_VIEW_MASK: data stored in extra storage with linked-listed memory blocks started with this
			DATA_VIEW_MASK = ((size_t)1 << (sizeof(size_t) * 8 - 2))
		};

		TBuffer() : size(0) {
			static_assert(N >= 3 * sizeof(size_t), "Must has stock storage of at least 3 pointer size.");
#if defined(_MSC_VER) && _MSC_VER > 1200
			static_assert(std::is_trivially_constructible<T>::value, "Must be trivially constructible.");
			static_assert(std::is_trivially_destructible<T>::value, "Must be trivially destructible.");
#endif
		}

		TBuffer(size_t initSize) : size(0) {
			if (initSize != 0) {
				Resize(initSize);
			}
		}

		TBuffer(const T* p, size_t initSize) : size(0) {
			if (initSize != 0) {
				Resize(initSize);
				memcpy(GetData(), p, initSize * sizeof(T));
			}
		}

		TBuffer(const TBuffer& rhs) : size(0) {
			Copy(rhs);
		}

		~TBuffer() {
			if (IsManagedStorage()) {
				assert(buffer != nullptr);
				free(buffer);
			}
		}

		void Clear() {
			if (IsManagedStorage()) {
				assert(buffer != nullptr);
				free(buffer);
			}

			size = 0;
		}

		TBuffer& operator = (const std::basic_string<T, std::char_traits<T>, std::allocator<T> >& str) {
			Resize(str.size());
			memcpy(GetData(), str.data(), str.size() * sizeof(T));
			return *this;
		}

		TBuffer& operator = (const TBuffer& rhs) {
			Copy(rhs);
			return *this;
		}

#if defined(_MSC_VER) && _MSC_VER <= 1200
		explicit
#endif
		TBuffer(rvalue<TBuffer> rv) {
			TBuffer& rhs = rv;
			memcpy(this, &rhs, sizeof(*this));
			rhs.size = 0;
		}

		TBuffer& operator = (rvalue<TBuffer> rv) {
			Clear();

			TBuffer& rhs = rv;
			memcpy(this, &rhs, sizeof(*this));
			rhs.size = 0;

			return *this;
		}

		static TBuffer& Null() {
			static TBuffer empty;
			return empty;
		}

		// Danger! Be aware at your own risk!
		static TBuffer View(T* data, size_t length) {
			TBuffer buffer;
			buffer.size = length | (EXT_STORE_MASK | DATA_VIEW_MASK);
			buffer.buffer = data;
			buffer.next = nullptr;
			buffer.tail = nullptr;

			return buffer;
		}

		bool IsManagedStorage() const { return (size & (DATA_VIEW_MASK | EXT_STORE_MASK)) == EXT_STORE_MASK; }
		bool IsViewStorage() const { return !!(size & DATA_VIEW_MASK); }
		bool IsStockStorage() const { return !(size & EXT_STORE_MASK); }
		size_t GetSize() const { assert(size <= N || (size & ~EXT_STORE_MASK) > N); return size & ~(EXT_STORE_MASK | DATA_VIEW_MASK); }
		const T* GetData() const { return IsStockStorage() ? stockStorage : buffer; }
		T* GetData() { return IsStockStorage() ? stockStorage : buffer; }

		size_t GetViewSize() const {
			if (IsViewStorage()) {
				const TBuffer* p = this;
				size_t s = 0;
				while (p != nullptr) {
					s += p->GetSize();
					p = p->next;
				}

				return s;
			} else {
				return GetSize();
			}
		}

		// Import data from given pointer & size
		void Import(size_t offset, const T* ptr, size_t size, size_t repeat = 1) {
			if (IsViewStorage()) {
				TBuffer* p = this;
				assert(offset + size * repeat <= GetViewSize());
				while (repeat-- != 0) {
					size_t k = 0;
					while (p != nullptr && k < size) {
						size_t len = p->GetSize();

						// not enough, just copy part of data
						if (offset < len) {
							size_t r = Math::Min(len - offset, size - k);
							memcpy(p->GetData() + offset, ptr + k, r * sizeof(T));
							k += r;
							offset = 0; // go next node
						} else {
							offset -= len;
						}

						p = p->next;
					}
				}
			} else {
				T* buffer = GetData();
				while (repeat-- != 0) {
					memcpy(buffer + offset, ptr, size * sizeof(T));
					offset += size;
				}
			}
		}

		// Import data from another buffer object
		void Import(size_t dstOffset, const TBuffer& buffer, size_t repeat = 1) {
			if (buffer.IsViewStorage()) {
				if (IsViewStorage()) { // both source and destination are view storage
					assert(GetViewSize() >= dstOffset + buffer.GetViewSize() * repeat);
					TBuffer* p = this;

					while (repeat-- != 0) {
						const TBuffer* q = &buffer;
						size_t srcSize = q->GetSize();
						const T* src = q->GetData();
						size_t dstSize = p->GetSize();
						T* dst = p->GetData();
						size_t srcOffset = 0;

						do {
							if (dstOffset < dstSize) {
								// get safe size for copy
								size_t r = Math::Min(dstSize - dstOffset, srcSize - srcOffset);
								memcpy(dst + dstOffset, src + srcOffset, r * sizeof(T));
								dstOffset += r;
								srcOffset += r;

								// go next for dst
								if (dstOffset >= dstSize) {
									p = p->next;
									if (p != nullptr) {
										dstSize = p->GetSize();
										dst = p->GetData();
										dstOffset = 0;
									}
								}

								// go next for src
								if (srcOffset >= srcSize) {
									q = q->next;
									if (q != nullptr) {
										srcSize = q->GetSize();
										src = q->GetData();
										srcOffset = 0;
									}
								}
							} else {
								dstOffset -= dstSize;
								p = p->next;
							}
						} while (p != nullptr && q != nullptr); // data end
					}
				} else {
					// only destination is view storage
					assert(GetSize() >= dstOffset + buffer.GetViewSize() * repeat);
					T* target = GetData() + dstOffset;

					while (repeat-- != 0) {
						const TBuffer* p = &buffer;

						while (p != nullptr) {
							size_t size = p->GetSize();
							memcpy(target, p->GetData(), size * sizeof(T));
							target += size;
							p = p->next;
						}
					}
				}
			} else {
				// go continous importing
				Import(dstOffset, buffer.GetData(), buffer.GetSize(), repeat);
			}
		}

		// Check if empty. notice that DATA_VIEW_MASK and EXT_STORE_MASK must be zero
		bool Empty() const { return size == 0; }
		bool operator == (const TBuffer& rhs) const {
			assert(IsViewStorage() == rhs.IsViewStorage());
			assert(!IsViewStorage() || (next == nullptr && rhs.next == nullptr));
			if (size != rhs.size) return false;
			if (size == 0) return true;

			return memcmp(GetData(), rhs.GetData(), GetSize() * sizeof(T)) == 0;
		}

		bool operator < (const TBuffer& rhs) const {
			assert(IsViewStorage() == rhs.IsViewStorage());
			assert(!IsViewStorage() || (next == nullptr && rhs.next == nullptr));
			if (size == 0) {
				return rhs.size != 0;
			} else {
				bool less = size < rhs.size;
				size_t minSize = (less ? size : rhs.size) & (~(EXT_STORE_MASK | DATA_VIEW_MASK));
				int result = memcmp(GetData(), rhs.GetData(), minSize * sizeof(T));
				return result != 0 ? result < 0 : less; // determinated by data size if common part is equal
			}
		}

		const T& operator [] (size_t index) const {
			assert(index < GetSize());
			return GetData()[index];
		}

		T& operator [] (size_t index) {
			assert(index < GetSize());
			return GetData()[index];
		}

		void Resize(size_t s, const T& init) {
			size_t orgSize = GetSize();
			Resize(s);

			if (s > orgSize) {
				T* ptr = GetData();
				std::fill(ptr + orgSize, ptr + s, init);
			}
		}

		void Resize(size_t s) {
			assert(!IsViewStorage());
			if (IsStockStorage()) {
				if (s > N) { // out of bound
					T* newBuffer = reinterpret_cast<T*>(malloc(s * sizeof(T)));
					memcpy(newBuffer, stockStorage, GetSize() * sizeof(T));
					buffer = newBuffer;
					size = s | EXT_STORE_MASK;
				} else {
					size = s;
				}
			} else {
				if (s > N) {
					if (s > GetSize()) {
						buffer = reinterpret_cast<T*>(realloc(buffer, s * sizeof(T)));
					}

					size = s | EXT_STORE_MASK;
				} else {
					T* orgBuffer = buffer;
					memcpy(stockStorage, orgBuffer, s * sizeof(T));
					free(orgBuffer);

					size = s;
				}
			}
		}

		void Swap(TBuffer& rhs) {
			std::swap(size, rhs.size);
			for (size_t i = 0; i < N; i++) {
				std::swap(stockStorage[i], rhs.stockStorage[i]);
			}
		}

		TBuffer& Append(const TBuffer& rhs) {
			if (Empty()) {
				*this = rhs;
				return *this;
			} else if (IsViewStorage()) {
				// source is view storage, destination is continous
				assert(rhs.IsViewStorage());
				TBuffer* p = this;

				while (true) {
					size_t curSize = p->GetSize();
					if (curSize == 0) {
						*p = rhs;
						return *this;
					} else if (rhs.buffer == p->buffer + curSize && p->next == nullptr) { // continuous?
						p->size += rhs.GetSize();
						p->next = rhs.next;
						tail = rhs.tail == nullptr ? tail : rhs.tail;
						return *this;
					} else {
						if (p->tail == nullptr) {
							assert(p->next == nullptr);
							p->next = const_cast<TBuffer*>(&rhs);
							tail = rhs.tail == nullptr ? p->next : rhs.tail;
							return *this;
						} else {
							p = p->tail;
						}
					}
				}

				// never reach here
				return *this;
			} else {
				assert(!rhs.IsViewStorage() || rhs.next == nullptr);
				return Append(rhs.GetData(), rhs.GetSize());
			}
		}

		TBuffer& Append(const T* buffer, size_t appendSize) {
			if (appendSize != 0) {
				size_t orgSize = GetSize();
				Resize(orgSize + appendSize);
				memcpy(GetData() + orgSize, buffer, appendSize * sizeof(T));
			}

			return *this;
		}

		TBuffer& Assign(const T* buffer, size_t n) {
			Resize(n);
			if (n != 0) {
				memcpy(GetData(), buffer, n * sizeof(T));
			}

			return *this;
		}

	protected:
		void Copy(const TBuffer& rhs) {
			if (rhs.IsViewStorage()) {
				Clear();
				memcpy(this, &rhs, sizeof(rhs));
			} else {
				size_t s = rhs.GetSize();
				Resize(s);
				memcpy(GetData(), rhs.GetData(), s * sizeof(T));
			}
		}

		size_t size;
		union {
			struct {
				T* buffer;
				TBuffer* next;
				TBuffer* tail;
			};
			T stockStorage[N];
		};
	};

	typedef TBuffer<uint8_t> Bytes;

#define StaticBytes(f) Bytes((const uint8_t*)#f, sizeof(#f) - 1)
}

