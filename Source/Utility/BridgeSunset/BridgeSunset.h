// BridgeSunset.h -- Asynchronous routine dispatching module for script
// PaintDream (paintdream@paintdream.com)
// 2015-1-2
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../Core/System/Kernel.h"
#include "../../Core/System/TaskGraph.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"
#include <bitset>

namespace PaintsNow {
	class SharedContext : public TReflected<SharedContext, SharedTiny> {
	public:
		SharedContext();
		~SharedContext() override;

		std::atomic<size_t> atomicValue;
		std::atomic<SharedContext*> next;
		std::vector<TShared<SharedTiny> > objectVector;
	};

	class BridgeSunset : public TReflected<BridgeSunset, IScript::Library>, public IScript::RequestPool, public ISyncObject {
	public:
		BridgeSunset(IThread& threadApi, IScript& script, uint32_t threadCount, uint32_t warpCount, long balancer);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~BridgeSunset() override;
		void ScriptInitialize(IScript::Request& request) override;
		void ScriptUninitialize(IScript::Request& request) override;
		void Reset();
		Kernel& GetKernel();
		ThreadPool& GetThreadPool();
		IStreamBase& LogError();
		IStreamBase& LogInfo();
		bool IsExiting() const;
		uint32_t AllocateWarpIndex();
		void FreeWarpIndex(uint32_t warp);

	protected:
		/// <summary>
		/// Create a new shared context
		/// </summary>
		/// <returns> shared context object </returns>
		TShared<SharedContext> RequestNewSharedContext(IScript::Request& request);

		/// <summary>
		/// Set counter of shared context atomically, return the old value
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="counter"> the new counter </param>
		/// <returns> old value of counter </returns>
		size_t RequestSetSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t counter);

		/// <summary>
		/// Compare exchange counter of shared context atomically 
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="comparedTo"> the compared counter value </param>
		/// <param name="counter"> the new counter value </param>
		/// <returns> true if success </returns>
		bool RequestCompareExchangeSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t comparedTo, size_t counter);

		/// <summary>
		/// Add counter of shared context atomically 
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="counter"> the delta new counter value </param>
		/// <returns> old value of counter </returns>
		size_t RequestAddSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t counter);

		/// <summary>
		/// Sub counter of shared context atomically 
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="counter"> the delta new counter value </param>
		/// <returns> old value of counter </returns>
		size_t RequestSubSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t counter);

		/// <summary>
		/// Get counter of shared context
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <returns> the value of counter </returns>
		size_t RequestGetSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext);

		/// <summary>
		/// Set object vector of shared context, this function is not thread-safe
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="tinies"> the vector of tiny to be set </param>
		void RequestSetSharedContextObjects(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, std::vector<IScript::Delegate<SharedTiny> >& tinies);

		/// <summary>
		/// Get objects of shared context, this function is not thread-safe with SetSharedContextObject
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="index"> the index </param>
		/// <returns> TaskGraph object </returns>
		const std::vector<TShared<SharedTiny> >& RequestGetSharedContextObjects(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext);

		/// <summary>
		/// Set object of shared context with given index, this function is not thread-safe with Get/SetSharedContextObject(s) with the same index
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="tiny"> the tiny to be set </param>
		void RequestSetSharedContextObject(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t index, IScript::Delegate<SharedTiny> tiny);

		/// <summary>
		/// Get object of shared context with given index, this function is not thread-safe with SetSharedContextObject(s) with the same index
		/// </summary>
		/// <param name="sharedContext"> the shared context </param>
		/// <param name="tiny"> the tiny to be set </param>
		/// <returns> the object </returns>
		TShared<SharedTiny> RequestGetSharedContextObject(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t index);

		/// <summary>
		/// Chain context to another atomically
		/// </summary>
		/// <param name="sentinelSharedContext"> the sentinel shared context with 'next' field as head pointer </param>
		/// <param name="otherSharedContext"> the other shared context </param>
		void RequestChainSharedContext(IScript::Request& request, IScript::Delegate<SharedContext> sentinelSharedContext, IScript::Delegate<SharedContext> sharedContext);

		/// <summary>
		/// Extract context chain
		/// </summary>
		/// <param name="sentinelSharedContext"> the sentinel shared context </param>
		/// <returns> all shared context in chain </returns>
		std::vector<TShared<SharedContext> > RequestExtractSharedContextChain(IScript::Request& request, IScript::Delegate<SharedContext> sentinelSharedContext);

		/// <summary>
		/// Create a new task graph
		/// </summary>
		/// <param name="statupWarp"> startup warp index of graph </param>
		/// <returns> TaskGraph object </returns>
		TShared<TaskGraph> RequestNewGraph(IScript::Request& request, int32_t startupWarp);

		/// <summary>
		/// Queue a new task to an existing task graph
		/// </summary>
		/// <param name="graph"> TaskGraph object </param>
		/// <param name="tiny"> task tiny object </param>
		/// <param name="callback"> task callback </param>
		/// <returns> Queued task id </returns>
		int32_t RequestQueueGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, IScript::Delegate<WarpTiny> tiny, IScript::Request::Ref callback);

		/// <summary>
		/// Set execution dependency between two tasks.
		/// </summary>
		/// <param name="graph"> TaskGraph object </param>
		/// <param name="prev"> pre task </param>
		/// <param name="next"> post task </param>
		/// <returns></returns>
		void RequestConnectGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, int32_t prev, int32_t next);

		/// <summary>
		/// Execute TaskGraph at once
		/// </summary>
		/// <param name="graph"> Execute TaskGraph at once </param>
		/// <param name="callback"> Callback on all task finished. </param>
		/// <returns></returns>
		void RequestExecuteGraph(IScript::Request& request, IScript::Delegate<TaskGraph> graph, IScript::Request::Ref callback);

		/// <summary>
		/// Queue a routine based on a tiny
		/// </summary>
		/// <param name="tiny"> the tiny object </param>
		/// <param name="callback"> callback </param>
		/// <returns></returns>
		void RequestQueueRoutine(IScript::Request& request, IScript::Delegate<WarpTiny> tiny, IScript::Request::Ref callback);

		/// <summary>
		/// Get count of warps
		/// </summary>
		/// <returns> The count of warps </returns>
		uint32_t RequestGetWarpCount(IScript::Request& request);

		/// <summary>
		/// Update tiny warp (danger!)
		/// </summary>
		/// <param name="source"> the source object </param>
		/// <param name="index"> the new warp index </param>
		void RequestSetWarpIndex(IScript::Request& request, IScript::Delegate<WarpTiny> source, uint32_t index);

		/// <summary>
		/// Get tiny warp
		/// </summary>
		/// <param name="source"> the source object </param>
		/// <returns> the warp index of source object </returns>
		uint32_t RequestGetWarpIndex(IScript::Request& request, IScript::Delegate<WarpTiny> source);

		/// <summary>
		/// Get current thread index
		/// </summary>
		/// <returns> the thread index of current thread </returns>
		uint32_t RequestGetCurrentThreadIndex(IScript::Request& request);

		/// <summary>
		/// Get current warp index
		/// </summary>
		/// <returns> the warp index of current thread </returns>
		uint32_t RequestGetCurrentWarpIndex(IScript::Request& request);

		/// <summary>
		/// Get null warp index
		/// </summary>
		/// <returns> the warp index of null </returns>
		uint32_t RequestGetNullWarpIndex(IScript::Request& request);

		/// <summary>
		/// Allocate warp index
		/// </summary>
		/// <returns> the new warp index </returns>
		uint32_t RequestAllocateWarpIndex(IScript::Request& request);

		/// <summary>
		/// Free warp index
		/// </summary>
		/// <param name="warpIndex"> the warp index to free </param>
		void RequestFreeWarpIndex(IScript::Request& request, uint32_t warpIndex);

		/// <summary>
		/// Set priority for warp
		/// </summary>
		/// <param name="warpIndex"> the warp index to set </param>
		/// <param name="priority"> the priority </param>
		void RequestSetWarpPriority(IScript::Request& request, uint32_t warpIndex, uint32_t priority);

		/// <summary>
		/// Pin tiny content
		/// </summary>
		/// <param name="source"> the source object </param>
		void RequestPin(IScript::Request& request, IScript::Delegate<WarpTiny> source);

		/// <summary>
		/// Unpin tiny content
		/// </summary>
		/// <param name="source"> the source object </param>
		void RequestUnpin(IScript::Request& request, IScript::Delegate<WarpTiny> source);

		/// <summary>
		/// Clone an object
		/// </summary>
		/// <param name="source"> the source object </param>
		/// <returns> The cloned object </returns>
		TShared<SharedTiny> RequestClone(IScript::Request& request, IScript::Delegate<SharedTiny> source);

	protected:
		ThreadPool threadPool;
		Kernel kernel;
		IStreamBase* logErrorStream;
		IStreamBase* logInfoStream;
		std::atomic<size_t> exiting;

		enum { WARP_VAR_COUNT = (1 << WarpTiny::WARP_BITS) / (sizeof(size_t) * 8) };
		size_t warpBitset[WARP_VAR_COUNT];
	};

#define CHECK_THREAD_IN_LIBRARY(warpTiny) \
	(MUST_CHECK_REFERENCE_ONCE); \
	if (bridgeSunset.GetKernel().GetCurrentWarpIndex() != warpTiny->GetWarpIndex()) { \
		request.Error("Threading routine failed on " #warpTiny); \
		assert(false); \
	}

	template <bool deref>
	class ScriptTaskTemplateBase : public TaskOnce {
	public:
#if defined(_MSC_VER) && _MSC_VER <= 1200
		typedef ScriptTaskTemplateBase<deref> BaseClass;
#endif
		ScriptTaskTemplateBase(IScript::Request::Ref ref) : callback(ref) {}

		template <class T>
		void Delete(void* context, T* t) {
			if (deref) {
				BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
				if (!bridgeSunset.IsExiting()) {
					IScript::Request& req = bridgeSunset.GetScript().GetDefaultRequest();
					req.DoLock();
					req.Dereference(callback);
					req.UnLock();
				}
			}

			ITask::Delete(t);
		}

		IScript::Request::Ref callback;
	};

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <bool deref>
	class ScriptTaskTemplate : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplate(IScript::Request::Ref ref) : ScriptTaskTemplateBase<deref>(ref) {}

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}
	};

	inline ITask* CreateTaskScript(IScript::Request::Ref ref) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplate<false>))) ScriptTaskTemplate<false>(ref);
	}

	inline ITask* CreateTaskScriptOnce(IScript::Request::Ref ref) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplate<true>))) ScriptTaskTemplate<true>(ref);
	}

	template <bool deref, class A>
	class ScriptTaskTemplateA : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateA(IScript::Request::Ref ref, const A& a) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback, pa);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		typename std::decay<A>::type pa;
	};

	template <class A>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateA<false, A>))) ScriptTaskTemplateA<false, A>(ref, a);
	}

	template <class A>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateA<true, A>))) ScriptTaskTemplateA<true, A>(ref, a);
	}

	template <bool deref, class A, class B>
	class ScriptTaskTemplateB : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateB(IScript::Request::Ref ref, const A& a, const B& b) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback, pa, pb);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
	};

	template <class A, class B>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateB<false, A, B>))) ScriptTaskTemplateB<false, A, B>(ref, a, b);
	}

	template <class A, class B>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateB<true, A, B>))) ScriptTaskTemplateB<true, A, B>(ref, a, b);
	}

	template <bool deref, class A, class B, class C>
	class ScriptTaskTemplateC : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateC(IScript::Request::Ref ref, const A& a, const B& b, const C& c) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback, pa, pb, pc);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
	};

	template <class A, class B, class C>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateC<false, A, B, C>))) ScriptTaskTemplateC<false, A, B, C>(ref, a, b, c);
	}

	template <class A, class B, class C>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateC<true, A, B, C>))) ScriptTaskTemplateC<true, A, B, C>(ref, a, b, c);
	}

	template <bool deref, class A, class B, class C, class D>
	class ScriptTaskTemplateD : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateD(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback, pa, pb, pc, pd);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
	};

	template <class A, class B, class C, class D>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateD<false, A, B, C, D>))) ScriptTaskTemplateD<false, A, B, C, D>(ref, a, b, c, d);
	}

	template <class A, class B, class C, class D>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateD<true, A, B, C, D>))) ScriptTaskTemplateD<true, A, B, C, D>(ref, a, b, c, d);
	}

	template <bool deref, class A, class B, class C, class D, class E>
	class ScriptTaskTemplateE : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateE(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback, pa, pb, pc, pd, pe);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();

				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
	};

	template <class A, class B, class C, class D, class E>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateE<false, A, B, C, D, E>))) ScriptTaskTemplateE<false, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <class A, class B, class C, class D, class E>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateE<true, A, B, C, D, E>))) ScriptTaskTemplateE<true, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <bool deref, class A, class B, class C, class D, class E, class F>
	class ScriptTaskTemplateF : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateF(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback, pa, pb, pc, pd, pe, pf);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
	};

	template <class A, class B, class C, class D, class E, class F>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateF<false, A, B, C, D, E, F>))) ScriptTaskTemplateF<false, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <class A, class B, class C, class D, class E, class F>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateF<true, A, B, C, D, E, F>))) ScriptTaskTemplateF<true, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <bool deref, class A, class B, class C, class D, class E, class F, class G>
	class ScriptTaskTemplateG : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateG(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) : ScriptTaskTemplateBase<deref>(ref) {
			pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); pg = const_cast<G&>(g);
		}

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				req.Call(callback, pa, pb, pc, pd, pe, pf, pg);
				req.Pop();
				if (deref) {
					req.Dereference(callback);
				}
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
		typename std::decay<G>::type pg;
	};

	template <class A, class B, class C, class D, class E, class F, class G>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateG<false, A, B, C, D, E, F, G>))) ScriptTaskTemplateG<false, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}

	template <class A, class B, class C, class D, class E, class F, class G>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new (ITask::Allocate(sizeof(ScriptTaskTemplateG<true, A, B, C, D, E, F, G>))) ScriptTaskTemplateG<true, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}

	// Routine tasks

	template <class T>
	class ScriptHandlerTemplate : public TaskOnce {
	public:
		ScriptHandlerTemplate(T ref) : callback(ref) {}
		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(req);
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
	};

	template <class T>
	ITask* CreateTaskScriptHandler(T ref) {
		return new ScriptHandlerTemplate(ref);
	}

	template <class T, class A>
	class ScriptHandlerTemplateA : public TaskOnce {
	public:
		ScriptHandlerTemplateA(T ref, const A& a) : callback(ref) { pa = const_cast<A&>(a); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(req, pa);
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
	};

	template <class T, class A>
	ITask* CreateTaskScriptHandler(T ref, const A& a) {
		return new (ITask::Allocate(sizeof(ScriptHandlerTemplateA<T, A>))) ScriptHandlerTemplateA<T, A>(ref, a);
	}

	template <class T, class A, class B>
	class ScriptHandlerTemplateB : public TaskOnce {
	public:
		ScriptHandlerTemplateB(T ref, const A& a, const B& b) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(req, pa, pb);
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
	};

	template <class T, class A, class B>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b) {
		return new (ITask::Allocate(sizeof(ScriptHandlerTemplateB<T, A, B>))) ScriptHandlerTemplateB<T, A, B>(ref, a, b);
	}

	template <class T, class A, class B, class C>
	class ScriptHandlerTemplateC : public TaskOnce {
	public:
		ScriptHandlerTemplateC(T ref, const A& a, const B& b, const C& c) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(req, pa, pb, pc);

				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
	};

	template <class T, class A, class B, class C>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c) {
		return new (ITask::Allocate(sizeof(ScriptHandlerTemplateC<T, A, B, C>))) ScriptHandlerTemplateC<T, A, B, C>(ref, a, b, c);
	}

	template <class T, class A, class B, class C, class D>
	class ScriptHandlerTemplateD : public TaskOnce {
	public:
		ScriptHandlerTemplateD(T ref, const A& a, const B& b, const C& c, const D& d) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(pa, pb, pc, pd);

				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
	};

	template <class T, class A, class B, class C, class D>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d) {
		return new (ITask::Allocate(sizeof(ScriptHandlerTemplateD<T, A, B, C, D>))) ScriptHandlerTemplateD<T, A, B, C, D>(ref, a, b, c, d);
	}

	template <class T, class A, class B, class C, class D, class E>
	class ScriptHandlerTemplateE : public TaskOnce {
	public:
		ScriptHandlerTemplateE(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(req, pa, pb, pc, pd, pe);

				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
	};

	template <class T, class A, class B, class C, class D, class E>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new (ITask::Allocate(sizeof(ScriptHandlerTemplateE<T, A, B, C, D, E>))) ScriptHandlerTemplateE<T, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <class T, class A, class B, class C, class D, class E, class F>
	class ScriptHandlerTemplateF : public TaskOnce {
	public:
		ScriptHandlerTemplateF(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = f; }

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(req, pa, pb, pc, pd, pe, pf);
			}

			bridgeSunset.requestPool.ReleaseSafe(&req);
			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
	};

	template <class T, class A, class B, class C, class D, class E, class F>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new (ITask::Allocate(sizeof(ScriptHandlerTemplateF<T, A, B, C, D, E, F>))) ScriptHandlerTemplateF<T, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	class ScriptHandlerTemplateG : public TaskOnce {
	public:
		ScriptHandlerTemplateG(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) : callback(ref) {
			pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); pg = const_cast<G&>(g);
		}
		
		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				callback(req, pa, pb, pc, pd, pe, pf, pg);

				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
		typename std::decay<F>::type pg;
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new (ITask::Allocate(sizeof(ScriptHandlerTemplateG<T, A, B, C, D, E, F, G>))) ScriptHandlerTemplateG<T, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}

#else
	template <bool deref, typename... Args>
	class ScriptTaskTemplate : public ScriptTaskTemplateBase<deref> {
	public:
		typedef ScriptTaskTemplateBase<deref> BaseClass;
		template <typename... Params>
		ScriptTaskTemplate(IScript::Request::Ref c, Params&&... params) : BaseClass(c), arguments(std::forward<Params>(params)...) {}

		template <typename T, size_t index>
		struct Writer {
			void operator () (IScript::Request& request, T& arg) {
				request << std::get<std::tuple_size<T>::value - index>(arg);
				Writer<T, index - 1>()(request, arg);
			}
		};

		template <typename T>
		struct Writer<T, 0> {
			void operator () (IScript::Request& request, T& arg) {}
		};

		void Execute(void* context) override {
			OPTICK_EVENT();

			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				req.DoLock();
				req.Push();
				Writer<decltype(arguments), sizeof...(Args)>()(req, arguments);
				req.Call(BaseClass::callback);
				if (deref) {
					req.Dereference(BaseClass::callback);
				}
				req.Pop();
				req.UnLock();
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			BaseClass::Delete(context, this);
		}

		void Abort(void* context) override {
			BaseClass::Delete(context, this);
		}

		std::tuple<typename std::decay<Args>::type...> arguments;
	};

	template <typename... Args>
	ITask* CreateTaskScript(IScript::Request::Ref ref, Args&&... args) {
		void* p = ITask::Allocate(sizeof(ScriptTaskTemplate<false, Args...>));
		return new (p) ScriptTaskTemplate<false, Args...>(ref, std::forward<Args>(args)...);
	}

	template <typename... Args>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, Args&&... args) {
		void* p = ITask::Allocate(sizeof(ScriptTaskTemplate<true, Args...>));
		return new (p) ScriptTaskTemplate<true, Args...>(ref, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	class ScriptHandlerTemplate : public TaskOnce {
	public:
		template <typename C, typename... Params>
		ScriptHandlerTemplate(C&& c, Params&&... params) : callback(std::forward<C>(c)), arguments(std::forward<Params>(params)...) {}

		template <size_t... S>
		void Apply(IScript::Request& context, seq<S...>) {
			callback(context, std::move(std::get<S>(arguments))...);
		}

		void Execute(void* context) override {
			OPTICK_EVENT();

			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			if (!bridgeSunset.IsExiting()) {
				IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
				Apply(req, gen_seq<sizeof...(Args)>());
				bridgeSunset.requestPool.ReleaseSafe(&req);
			}

			ITask::Delete(this);
		}

		void Abort(void* context) override {
			ITask::Delete(this);
		}

		T callback;
		std::tuple<typename std::decay<Args>::type...> arguments;
	};

	template <typename T, typename... Args>
	ITask* CreateTaskScriptHandler(T&& t, Args&&... args) {
		void* p = ITask::Allocate(sizeof(ScriptHandlerTemplate<typename std::decay<T>::type, Args...>));
		return new (p) ScriptHandlerTemplate<typename std::decay<T>::type, Args...>(std::forward<T>(t), std::forward<Args>(args)...);
	}

#endif		
}
