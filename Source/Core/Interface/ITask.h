// ITask.h
// PaintDream (paintdream@paintdream.com)
// 2016-4-6
//

#pragma once
#include "IType.h"
#include "../Template/TAlgorithm.h"
#include "../Template/TAtomic.h"

namespace PaintsNow {
	class pure_interface ITask {
	public:
		ITask();
		virtual ~ITask();
		virtual void Execute(void* context) = 0;
		virtual void Suspend(void* context) = 0;
		virtual void Resume(void* context) = 0;
		virtual void Abort(void* context) = 0;
		virtual bool Continue() const = 0;

		static void* Allocate(size_t taskMemorySize);
		static void Deallocate(void* p, size_t taskMemorySize);

		template <class T>
		void Delete(T* t) {
			t->~T();
			ITask::Deallocate(t, sizeof(T));
		}

		ITask* next;
		size_t queued;
	};

	class pure_interface TaskOnce : public ITask {
	public:
		void Execute(void* context) override = 0;
		void Suspend(void* context) override;
		void Resume(void* context) override;
		void Abort(void* context) override;
		bool Continue() const override;
	};

	class pure_interface TaskRepeat : public ITask {
	public:
		void Execute(void* context) override = 0;
		void Suspend(void* context) override;
		void Resume(void* context) override;
		void Abort(void* context) override = 0;
		bool Continue() const override;
	};

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <class T>
	class TaskTemplate : public TaskOnce {
	public:
		TaskTemplate(T ref) : callback(ref) {}
		void Execute(void* request) override {
			callback(request, true);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false);
			ITask::Delete(this);
		}

		T callback;
	};

	template <class T>
	ITask* CreateTask(T ref) {
		return new (ITask::Allocate(sizeof(TaskTemplate<T>))) TaskTemplate<T>(ref);
	}

	template <class T, class A>
	class TaskTemplateA : public TaskOnce {
	public:
		TaskTemplateA(T ref, const A& a) : callback(ref) { pa = const_cast<A&>(a); }
		void Execute(void* request) override {
			callback(request, true, pa);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false, pa);
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
	};

	template <class T, class A>
	ITask* CreateTask(T ref, const A& a) {
		return new (ITask::Allocate(sizeof(TaskTemplateA<T, A>))) TaskTemplateA<T, A>(ref, a);
	}

	template <class T, class A, class B>
	class TaskTemplateB : public TaskOnce {
	public:
		TaskTemplateB(T ref, const A& a, const B& b) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); }
		void Execute(void* request) override {
			callback(request, true, pa, pb);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false, pa, pb);
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
	};

	template <class T, class A, class B>
	ITask* CreateTask(T ref, const A& a, const B& b) {
		return new (ITask::Allocate(sizeof(TaskTemplateB<T, A, B>))) TaskTemplateB<T, A, B>(ref, a, b);
	}

	template <class T, class A, class B, class C>
	class TaskTemplateC : public TaskOnce {
	public:
		TaskTemplateC(T ref, const A& a, const B& b, const C& c) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); }
		void Execute(void* request) override {
			callback(request, true, pa, pb, pc);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false, pa, pb, pc);
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
	};

	template <class T, class A, class B, class C>
	ITask* CreateTask(T ref, const A& a, const B& b, const C& c) {
		return new (ITask::Allocate(sizeof(TaskTemplateC<T, A, B, C>))) TaskTemplateC<T, A, B, C>(ref, a, b, c);
	}

	template <class T, class A, class B, class C, class D>
	class TaskTemplateD : public TaskOnce {
	public:
		TaskTemplateD(T ref, const A& a, const B& b, const C& c, const D& d) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); }
		void Execute(void* request) override {
			callback(request, true, pa, pb, pc, pd);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false, pa, pb, pc, pd);
			ITask::Delete(this);
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
	};

	template <class T, class A, class B, class C, class D>
	ITask* CreateTask(T ref, const A& a, const B& b, const C& c, const D& d) {
		return new (ITask::Allocate(sizeof(TaskTemplateD<T, A, B, C, D>))) TaskTemplateD<T, A, B, C, D>(ref, a, b, c, d);
	}

	template <class T, class A, class B, class C, class D, class E>
	class TaskTemplateE : public TaskOnce {
	public:
		TaskTemplateE(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); }
		void Execute(void* request) override {
			callback(request, true, pa, pb, pc, pd, pe);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false, pa, pb, pc, pd, pe);
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
	ITask* CreateTask(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new (ITask::Allocate(sizeof(TaskTemplateE<T, A, B, C, D, E>))) TaskTemplateE<T, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <class T, class A, class B, class C, class D, class E, class F>
	class TaskTemplateF : public TaskOnce {
	public:
		TaskTemplateF(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); }
		void Execute(void* request) override {
			callback(request, true, pa, pb, pc, pd, pe, pf);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false, pa, pb, pc, pd, pe, pf);
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
	ITask* CreateTask(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new (ITask::Allocate(sizeof(TaskTemplateF<T, A, B, C, D, E, F>))) TaskTemplateF<T, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	class TaskTemplateG : public TaskOnce {
	public:
		TaskTemplateG(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); pg = const_cast<G&>(g); }
		void Execute(void* request) override {
			callback(request, true, pa, pb, pc, pd, pe, pf, pg);
			ITask::Delete(this);
		}
		void Abort(void* request) override {
			callback(request, false, pa, pb, pc, pd, pe, pf, pg);
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
	ITask* CreateTask(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new (ITask::Allocate(sizeof(TaskTemplateG<T, A, B, C, D, E, F, G>))) TaskTemplateG<T, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}

	// ContextFree tasks

	template <class T>
	class ContextFreeTaskTemplate : public TaskOnce {
	public:
		ContextFreeTaskTemplate(T ref) : callback(ref) {}
		void Execute(void* request) override {
			callback();
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			ITask::Delete(this);
		}

		T callback;
	};

	template <class T>
	ITask* CreateTaskContextFree(T ref) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplate<T>))) ContextFreeTaskTemplate<T>(ref);
	}

	template <class T, class A>
	class ContextFreeTaskTemplateA : public TaskOnce {
	public:
		ContextFreeTaskTemplateA(T ref, const A& a) : callback(ref) { pa = const_cast<A&>(a); }
		void Execute(void* request) override {
			callback(pa);
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			ITask::Delete(this);
		}
		T callback;
		typename std::decay<A>::type pa;
	};

	template <class T, class A>
	ITask* CreateTaskContextFree(T ref, const A& a) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplateA<T, A>))) ContextFreeTaskTemplateA<T, A>(ref, a);
	}

	template <class T, class A, class B>
	class ContextFreeTaskTemplateB : public TaskOnce {
	public:
		ContextFreeTaskTemplateB(T ref, const A& a, const B& b) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); }
		void Execute(void* request) override {
			callback(pa, pb);
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			ITask::Delete(this);
		}
		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
	};

	template <class T, class A, class B>
	ITask* CreateTaskContextFree(T ref, const A& a, const B& b) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplateB<T, A, B>))) ContextFreeTaskTemplateB<T, A, B>(ref, a, b);
	}

	template <class T, class A, class B, class C>
	class ContextFreeTaskTemplateC : public TaskOnce {
	public:
		ContextFreeTaskTemplateC(T ref, const A& a, const B& b, const C& c) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); }
		void Execute(void* request) override {
			callback(pa, pb, pc);
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			ITask::Delete(this);
		}
		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
	};

	template <class T, class A, class B, class C>
	ITask* CreateTaskContextFree(T ref, const A& a, const B& b, const C& c) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplateC<T, A, B, C>))) ContextFreeTaskTemplateC<T, A, B, C>(ref, a, b, c);
	}

	template <class T, class A, class B, class C, class D>
	class ContextFreeTaskTemplateD : public TaskOnce {
	public:
		ContextFreeTaskTemplateD(T ref, const A& a, const B& b, const C& c, const D& d) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); }
		void Execute(void* request) override {
			callback(pa, pb, pc, pd);
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			ITask::Delete(this);
		}
		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
	};

	template <class T, class A, class B, class C, class D>
	ITask* CreateTaskContextFree(T ref, const A& a, const B& b, const C& c, const D& d) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplateD<T, A, B, C, D>))) ContextFreeTaskTemplateD<T, A, B, C, D>(ref, a, b, c, d);
	}

	template <class T, class A, class B, class C, class D, class E>
	class ContextFreeTaskTemplateE : public TaskOnce {
	public:
		ContextFreeTaskTemplateE(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); }
		void Execute(void* request) override {
			callback(pa, pb, pc, pd, pe);
			ITask::Delete(this);
		}

		void Abort(void* request) override {
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
	ITask* CreateTaskContextFree(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplateE<T, A, B, C, D, E>))) ContextFreeTaskTemplateE<T, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <class T, class A, class B, class C, class D, class E, class F>
	class ContextFreeTaskTemplateF : public TaskOnce {
	public:
		ContextFreeTaskTemplateF(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); }
		void Execute(void* request) override {
			callback(pa, pb, pc, pd, pe, pf);
			ITask::Delete(this);
		}

		void Abort(void* request) override {
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
	ITask* CreateTaskContextFree(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplateF<T, A, B, C, D, E, F>))) ContextFreeTaskTemplateF<T, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	class ContextFreeTaskTemplateG : public TaskOnce {
	public:
		ContextFreeTaskTemplateG(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); pg = const_cast<G&>(g); }
		void Execute(void* request) override {
			callback(pa, pb, pc, pd, pe, pf, pg);
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			ITask::Delete(this);
		}
		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
		typename std::decay<G>::type pg;
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	ITask* CreateTaskContextFree(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new (ITask::Allocate(sizeof(ContextFreeTaskTemplateG<T, A, B, C, D, E, F, G>))) ContextFreeTaskTemplateG<T, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}
#else

	template <typename T, typename... Args>
	class TaskTemplate : public TaskOnce {
	public:
		template <typename C, typename... Params>
		TaskTemplate(C&& c, Params&&... params) : callback(std::forward<C>(c)), arguments(std::forward<Params>(params)...) {}

		template <size_t... S>
		void Apply(void* context, bool run, seq<S...>) {
			callback(context, run, std::move(std::get<S>(arguments))...);
		}

		void Execute(void* request) override {
			Apply(request, true, gen_seq<sizeof...(Args)>());
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			Apply(request, false, gen_seq<sizeof...(Args)>());
			ITask::Delete(this);
		}

		T callback;
		std::tuple<typename std::decay<Args>::type...> arguments;
	};

	template <typename T, typename... Args>
	ITask* CreateTask(T&& closure, Args&&... args) {
		void* p = ITask::Allocate(sizeof(TaskTemplate<typename std::decay<T>::type, Args...>));
		return new (p) TaskTemplate<typename std::decay<T>::type, Args...>(std::forward<T>(closure), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	class ContextFreeTaskTemplate : public TaskOnce {
	public:
		template <typename C, typename... Params>
		ContextFreeTaskTemplate(C&& c, Params&&... params) : callback(std::forward<C>(c)), arguments(std::forward<Params>(params)...) {}

		template <size_t... S>
		void Apply(seq<S...>) {
			callback(std::get<S>(arguments)...);
		}

		void Execute(void* request) override {
			Apply(gen_seq<sizeof...(Args)>());
			ITask::Delete(this);
		}

		void Abort(void* request) override {
			ITask::Delete(this);
		}

		T callback;
		std::tuple<typename std::decay<Args>::type...> arguments;
	};

	template <typename T, typename... Args>
	ITask* CreateTaskContextFree(T&& t, Args&&... args) {
		void* p = ITask::Allocate(sizeof(ContextFreeTaskTemplate<typename std::decay<T>::type, Args...>));
		return new (p) ContextFreeTaskTemplate<typename std::decay<T>::type, Args...>(std::forward<T>(t), std::forward<Args>(args)...);
	}
#endif
}

