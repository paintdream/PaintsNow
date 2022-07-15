// TaskGraph.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-30
//

#pragma once
#include "Kernel.h"
#include "Tiny.h"

namespace PaintsNow {
	class ThreadPool;

	// A task graph for executing tasks in toplogical order
	class TaskGraph : public TReflected<TaskGraph, SharedTiny> {
	public:
		TaskGraph(Kernel& kernel);
		~TaskGraph() override;

		// insert a new task, returns a integer that represents the node id
		size_t Insert(WarpTiny* host, ITask* task, void* overrideContext = nullptr);

		// add relation that 'to' must be executed after 'from' executed.
		void Next(size_t from, size_t to);

		// suspend a task temporarily, must call before target task actually runs
		void Suspend(size_t id);

		// resume a task
		void Resume(size_t id);

		// commit all tasks
		bool Dispatch(const TWrapper<void>& completion = nullptr);

		bool IsRunning() const;

	protected:
		class TaskNode : public ITask {
		public:
			void Execute(void* context) override;
			void Suspend(void* context) final;
			void Resume(void* context) final;
			void Abort(void* context) override;
			bool Continue() const final;

			TaskGraph* taskGraph;
			WarpTiny* host;
			ITask* task;
			void* overrideContext;
			size_t refCount;
			size_t totalRefCount;
			std::vector<TaskNode*> nextNodes;
		};

		friend class TaskNode;
		void Complete();

		alignas(CPU_CACHELINE_SIZE) Kernel& kernel;
		alignas(CPU_CACHELINE_SIZE) std::vector<TaskNode> taskNodes;
		alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> completedCount;
		alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> running;
		alignas(CPU_CACHELINE_SIZE) TWrapper<void> completion;
	};
}

