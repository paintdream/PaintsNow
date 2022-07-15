#pragma once
#include "../PaintsNow.h"
#include "IReflect.h"
#include <map>
#include <list>
#include <stack>

namespace PaintsNow {
	class pure_interface IStreamBase : public TReflected<IStreamBase, IReflectObjectComplex> {
	protected:
		~IStreamBase() override;

	public:
		IStreamBase();
		virtual void Flush() = 0;
		virtual bool Read(void* p, size_t& len) = 0;
		virtual bool Write(const void* p, size_t& len) = 0;
		virtual bool Transfer(IStreamBase& stream, size_t& len) = 0;
		virtual bool WriteDummy(size_t& len) = 0;
		enum SEEK_OPTION { BEGIN, CUR, END };
		virtual bool Seek(SEEK_OPTION option, int64_t offset) = 0;
		virtual bool Truncate(uint64_t length);
		virtual size_t Printf(const char* format, ...);
		virtual IStreamBase& GetBaseStream();
		void SetEnvironment(IReflectObject& object);
		IReflectObject& GetEnvironment();

		bool ReadBlock(void* p, size_t& len) {
			size_t org = len;
			if (Read(p, len)) {
				if (len == org) { // done
					return true;
				} else {
					Seek(IStreamBase::CUR, -(long)len);
					return false;
				}
			} else {
				return false;
			}
		}

		bool WriteBlock(const void* p, size_t& len) {
			size_t org = len;
			if (Write(p, len)) {
				if (len == org) {
					return true;
				} else {
					Seek(IStreamBase::CUR, -(long)len);
					return false;
				}
			} else {
				return false;
			}
		}

		class StreamState {
		public:
			StreamState(IStreamBase& s, bool su) : stream(s), success(su) {}
			template <class T>
			StreamState operator << (const T& t) {
				assert(success);
				if (success) {
					return stream << t;
				} else {
					return *this;
				}
			}

			template <class T>
			StreamState operator >> (const T& t) {
				assert(success);
				if (success) {
					return stream >> t;
				} else {
					return *this;
				}
			}

			operator bool() const {
				return success;
			}

			bool operator ! () const {
				return !success;
			}

		private:
			IStreamBase& stream;
			bool success;
		};

		template <class T>
		StreamState operator << (const T& t) {
			singleton Unique u = UniqueType<T>::Get();
#if defined(_MSC_VER) && _MSC_VER <= 1200
			singleton Unique ur = UniqueType<std::remove_pointer<T>::type>::Get();
#else
			singleton Unique ur = UniqueType<typename std::remove_pointer<T>::type>::Get();
#endif
			bool result = WriteForward(t, u, ur, (void*)&t, sizeof(T));
			return StreamState(*this, result);
		}

		template <class T>
		StreamState operator >> (T& t) {
			singleton Unique u = UniqueType<T>::Get();
#if defined(_MSC_VER) && _MSC_VER <= 1200
			singleton Unique ur = UniqueType<std::remove_pointer<T>::type>::Get();
#else
			singleton Unique ur = UniqueType<typename std::remove_pointer<T>::type>::Get();
#endif
			bool result = ReadForward(t, u, ur, (void*)&t, sizeof(T));
			return StreamState(*this, result);
		}

		virtual bool Write(IReflectObject& a, Unique type, void* ptr, size_t length);
		virtual bool Read(IReflectObject& a, Unique type, void* ptr, size_t length);

		template <class T>
		T Parse(UniqueType<T>) {
			T object;
			*this >> object;
			return object;
		}

	protected:
		bool WriteForward(const IReflectObject& a, Unique type, Unique refTypeID, void* ptr, size_t length);
		bool ReadForward(const IReflectObject& a, Unique type, Unique refTypeID, void* ptr, size_t length);

		IReflectObject* environment;
	};

	class StreamBaseMeasure : public IStreamBase {
	public:
		StreamBaseMeasure(IStreamBase& base);
		void Flush() override;
		bool Read(void* p, size_t& len) override;
		bool Write(const void* p, size_t& len) override;
		bool Transfer(IStreamBase& stream, size_t& len) override;
		bool WriteDummy(size_t& len) override;
		bool Seek(SEEK_OPTION option, int64_t offset) override;

		IStreamBase& stream;
		int64_t transition;
	};

	class MetaStreamPersist : public TReflected<MetaStreamPersist, MetaNodeBase> {
	public:
		virtual bool Read(IStreamBase& streamBase, void* ptr) const = 0;
		virtual bool Write(IStreamBase& streamBase, const void* ptr) const = 0;
		virtual String GetUniqueName() const = 0;
	};
}
