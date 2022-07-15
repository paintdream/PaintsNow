#pragma once
#include "../PaintsNow.h"
#include <string>
#include <exception>
#include <stdexcept>
#include <algorithm>

#if defined(_MSC_VER) && _MSC_VER <= 1200
	#include <emmintrin.h>
	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x500
	#endif
	
	#include "../Interface/IType.h"
	#include <windows.h>
	#include <winbase.h>
	
	namespace std {
		enum memory_order {
			memory_order_relaxed,
			memory_order_consume,
			memory_order_acquire,
			memory_order_release,
			memory_order_acq_rel,
			memory_order_seq_cst
		};
	}
	
	inline void YieldThread() {
		if (!SwitchToThread()) {
			for (int i = 0; i < 16; i++) {
				_mm_pause();
			}
		}
	}
	
	inline void YieldThreadFast() {
		_mm_pause();
	}
#else
	#include <atomic>
	#include <chrono>
	#include <thread>
	
	inline void YieldThread() {
		std::this_thread::yield();
	}
	
	#ifdef _MSC_VER
		#if defined(_M_IX86) || defined(_M_AMD64)
			#include <emmintrin.h>
			inline void YieldThreadFast() {
				_mm_pause();
			}
		#else
			inline void YieldThreadFast() {
				std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			}
		#endif
	#else
		#include <time.h>
		inline void YieldThreadFast() {
			timespec spec;
			spec.tv_sec = 0;
			spec.tv_nsec = 1;
			nanosleep(&spec, nullptr);
		}
	#endif
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
namespace std {
	// From Microsoft Visual C++ Header file.
	// implement std::atomic<> for Visual C++ 6.0
	// only support 32 bit atomic operations ..
	inline void atomic_thread_fence(std::memory_order order) {
		if (order != std::memory_order_relaxed) {
			MemoryBarrier();
		}
	}

	template <class T>
	class atomic {
	public:
		atomic(int32_t v = 0) : value(v) {}
	
		int32_t fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) {
			return InterlockedExchangeAdd(&value, (int32_t)arg);
		}

		int32_t fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) {
			return InterlockedExchangeAdd(&value, -(int32_t)arg);
		}

		int32_t fetch_or(T arg, std::memory_order order = std::memory_order_seq_cst) {
			LONG Old;

			do {
				Old = value;
			} while (InterlockedCompareExchange(&value,
				Old | (int32_t)arg,
				Old) != Old);

			return Old;
		}

		int32_t fetch_and(T arg, std::memory_order order = std::memory_order_seq_cst) {
			LONG Old;

			do {
				Old = value;
			} while (InterlockedCompareExchange(&value,
				Old & (int32_t)arg,
				Old) != Old);

			return Old;
		}

		int32_t fetch_xor(T arg, std::memory_order order = std::memory_order_seq_cst) {
			LONG Old;

			do {
				Old = value;
			} while (InterlockedCompareExchange(&value,
				Old ^ (int32_t)arg,
				Old) != Old);

			return Old;
		}

		T load(std::memory_order order = std::memory_order_acquire) const {
			T result = (T)value;
			std::atomic_thread_fence(order);
			return result;
		}

		void store(T v, std::memory_order order = std::memory_order_release) {
			if (order != std::memory_order_seq_cst) {
				std::atomic_thread_fence(order);
				value = (int32_t)v;
			} else {
				exchange(v, std::memory_order_seq_cst);
			}
		}

		T operator & (T t) const {
			return (T)(load(std::memory_order_acquire) & (int32_t)t);
		}

		T operator | (T t) const {
			return (T)(load(std::memory_order_acquire) | (int32_t)t);
		}

		T operator ^ (T t) const {
			return (T)(load(std::memory_order_acquire) ^ (int32_t)t);
		}

		T exchange(T t, std::memory_order order = std::memory_order_seq_cst) {
			return (T)InterlockedExchange(&value, (int32_t)t);
		}

		bool compare_exchange_strong(T& old, T u, std::memory_order success = std::memory_order_seq_cst, std::memory_order failure = std::memory_order_seq_cst) {
			T org = old;
			T result = (T)InterlockedCompareExchange((volatile long*)&value, (int32_t)u, (long)org);
			if (result == org) {
				return true;
			}

			old = result;
			return false;
		}

		bool compare_exchange_weak(T& old, T u, std::memory_order success = std::memory_order_seq_cst, std::memory_order failure = std::memory_order_seq_cst) {
			return compare_exchange_strong(old, u, success, failure);
		}

	private:
		volatile LONG value;
	};
}
#endif

namespace PaintsNow {
	template <class T>
	inline T SpinLock(std::atomic<T>& section, T newValue = 1) {
		T ret;
		while (true) {
			while (section.load(std::memory_order_acquire) == newValue) {
				YieldThreadFast();
			}

			if ((ret = section.exchange(newValue, std::memory_order_acquire)) != newValue) {
				break;
			}
		}
		
		return ret;
	}

	template <class T>
	inline void SpinUnLock(std::atomic<T>& section, T newValue = 0) {
		section.store(newValue, std::memory_order_release);
	}

	template <class T, T lockValue = 1, T unlockValue = 0>
	class TSpinLockGuard {
	public:
		TSpinLockGuard(std::atomic<T>& var) : variable(&var) {
			DoLock();
		}

		~TSpinLockGuard() {
			if (variable != nullptr) {
				UnLock();
			}
		}

		void DoLock() {
			assert(variable != nullptr);
			SpinLock(*variable, lockValue);
		}

		void UnLock() {
			assert(variable != nullptr);
			SpinUnLock(*variable, unlockValue);
			variable = nullptr;
		}

	private:
		TSpinLockGuard& operator = (const TSpinLockGuard& rhs);
		TSpinLockGuard(const TSpinLockGuard& rhs);

		std::atomic<T>* variable;
	};

	template <typename T = uint32_t>
	class TWriteFence {
	public:
#if defined(_MSC_VER) && _MSC_VER > 1200
		TWriteFence(std::atomic<T>& var, uint32_t& id) noexcept : variable(var), threadId(id) {
			Acquire(variable, threadId);
		}

		~TWriteFence() {
			Release(variable, threadId);
		}

		static void Acquire(std::atomic<T>& variable, uint32_t& threadId) noexcept {
			assert(variable.exchange(~(T)0, std::memory_order_acquire) == 0);
			threadId = IThread::GetCurrentNativeThreadId();
		}

		static void Release(std::atomic<T>& variable, uint32_t& threadId) noexcept {
			threadId = 0;
			assert(variable.exchange(0, std::memory_order_release) == ~(T)0);
		}

	private:
		std::atomic<T>& variable;
		uint32_t& threadId;
#endif
	};

	template <typename T = uint32_t>
	class TReadFence {
	public:
#if defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER > 1200
		TReadFence(std::atomic<T>& var, uint32_t& id) noexcept : variable(var), threadId(id) {
			Acquire(variable, threadId);
		}

		~TReadFence() {
			Release(variable, threadId);
		}

		static void Acquire(std::atomic<T>& variable, uint32_t& threadId) noexcept {
			assert(variable.fetch_add(1, std::memory_order_acquire) != ~(T)0);
			threadId = IThread::GetCurrentNativeThreadId();
		}

		static void Release(std::atomic<T>& variable, uint32_t& threadId) noexcept {
			threadId = 0;
			assert(variable.fetch_sub(1, std::memory_order_release) != ~(T)0);
		}

	private:
		std::atomic<T>& variable;
		uint32_t& threadId;
#endif
	};

	template <typename T = uint32_t>
	class TEnableReadWriteFence {
#if defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER > 1200
	private:
		mutable std::atomic<T> monitor;
		mutable uint32_t threadId;

	public:
		TEnableReadWriteFence() noexcept { monitor.store(0, std::memory_order_relaxed); }
		TReadFence<T> ReadFence() const noexcept {
			return TReadFence<T>(monitor, threadId);
		}

		TWriteFence<T> WriteFence() const noexcept {
			return TWriteFence<T>(monitor, threadId);
		}

		void AcquireRead() const noexcept {
			TReadFence<T>::Acquire(monitor, threadId);
		}

		void ReleaseRead() const noexcept {
			TReadFence<T>::Release(monitor, threadId);
		}

		void AcquireWrite() const noexcept {
			TWriteFence<T>::Acquire(monitor, threadId);
		}

		void ReleaseWrite() const noexcept {
			TWriteFence<T>::Release(monitor, threadId);
		}
#else
	public:
		TReadFence<T> ReadFence() const noexcept {
			return TReadFence<T>();
		}

		TWriteFence<T> WriteFence() const noexcept {
			return TWriteFence<T>();
		}

		void AcquireRead() const noexcept {}
		void ReleaseRead() const noexcept {}
		void AcquireWrite() const noexcept {}
		void ReleaseWrite() const noexcept {}
#endif
	};

	template <typename T = uint32_t>
	class TEnableInOutFence {
#if defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER > 1200
	private:
		mutable std::atomic<T> inMonitor;
		mutable std::atomic<T> outMonitor;
		mutable uint32_t inThreadId;
		mutable uint32_t outThreadId;

	public:
		TEnableInOutFence() noexcept {
			inMonitor.store(0, std::memory_order_relaxed);
			outMonitor.store(0, std::memory_order_relaxed);
			inThreadId = 0;
			outThreadId = 0;
		}

		TWriteFence<T> InFence() const noexcept {
			return TWriteFence<T>(inMonitor, inThreadId);
		}

		TWriteFence<T> OutFence() const noexcept {
			return TWriteFence<T>(outMonitor, outThreadId);
		}

		void AcquireIn() const noexcept {
			return TWriteFence<T>::Acquire(inMonitor, inThreadId);
		}

		void ReleaseIn() const noexcept {
			return TWriteFence<T>::Release(inMonitor, inThreadId);
		}

		void AcquireOut() const noexcept {
			return TWriteFence<T>::Acquire(outMonitor, outThreadId);
		}

		void ReleaseOut() const noexcept {
			return TWriteFence<T>::Release(outMonitor, outThreadId);
		}
#else
	public:
		TWriteFence<T> InFence() const noexcept {
			return TWriteFence<T>();
		}

		TWriteFence<T> OutFence() const noexcept {
			return TWriteFence<T>();
		}

		void AcquireIn() const noexcept {}
		void ReleaseIn() const noexcept {}
		void AcquireOut() const noexcept {}
		void ReleaseOut() const noexcept {}
#endif

	};


#if defined(_MSC_VER) && _MSC_VER > 1200
#define WRITE_FENCE_GUARD() TWriteFence<> writeGuard = WriteFence()
#define READ_FENCE_GUARD() TReadFence<> readGuard = ReadFence()
#define IN_FENCE_GUARD() TWriteFence<> inGuard = InFence()
#define OUT_FENCE_GUARD() TWriteFence<> outGuard = OutFence()
#define ENABLE_READ_WRITE_FENCE() , protected TEnableReadWriteFence<>
#define ENABLE_IN_OUT_FENCE() , protected TEnableInOutFence<>
#else
#define WRITE_FENCE_GUARD()
#define READ_FENCE_GUARD()
#define IN_FENCE_GUARD()
#define OUT_FENCE_GUARD()
#define ENABLE_READ_WRITE_FENCE()
#define ENABLE_IN_OUT_FENCE()
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <class T>
	class TSingleton {
	public:
		static T& Get() {
			static std::atomic<uint32_t> critical;
			static T* ptr = nullptr;
			if (ptr == nullptr) { // x86 is strongly ordered
				SpinLock(critical);
				if (ptr == nullptr) {
					static T object;
					ptr = &object;
				}

				SpinUnLock(critical); // unlock indicates std::memory_order_release
			}

			return *ptr;
		}

		operator T& () {
			return Get();
		}
	};
#else
	template <class T>
	class TSingleton {
	public:
		static T& Get() {
			// C++ 11 provides thread-safety for static variables.
			static T object;
			return object;
		}

		operator T& () {
			return Get();
		}
	};
#endif
}

