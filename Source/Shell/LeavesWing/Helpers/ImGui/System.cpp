#include "System.h"
#include "../../../../Utility/LeavesFlute/LeavesFlute.h"
#include "Core/imgui.h"
#include <iterator>

using namespace PaintsNow;

System::System() : statWindowDuration(500) {}

System::WarpStat::WarpStat(Kernel& k, int& stat) : taskPerFrame(0), kernel(k), expanded(true), lastClockStamp(0), statWindowDuration(stat), historyOffset(0), maxHistory(1.0f) {
	critical.store(0, std::memory_order_relaxed);
	memset(history, 0, sizeof(history));
}

System::WarpStat::~WarpStat() {}

static inline bool SortActiveWarpTinies(const std::pair<WarpTiny*, System::WarpStat::Record>& lhs, const std::pair<WarpTiny*, System::WarpStat::Record>& rhs) {
	return rhs.second.count < lhs.second.count;
}

class KernelSpy : public Kernel {
public:
	void Execute(System::WarpStat* stat) {
		uint32_t warpIndex = GetCurrentWarpIndex();

		Kernel::SubTaskQueue& taskQueue = taskQueueGrid[warpIndex];
		const std::vector<TaskQueue::RingBuffer>& ringBuffers = taskQueue.GetRingBuffers();

		for (size_t k = 0; k < ringBuffers.size(); k++) {
			const TaskQueue::RingBuffer& ringBuffer = ringBuffers[k];
			for (TaskQueue::RingBuffer::ConstIterator p = ringBuffer.Begin(); p != ringBuffer.End(); ++p) {
				const std::pair<ITask*, void*>& task = p.p->Get(p.it);
				WarpTiny* warpTiny = reinterpret_cast<WarpTiny*>(task.second);
				if (warpTiny != nullptr && warpTiny != stat) {
					System::WarpStat::Record& record = stat->activeWarpTinies[warpTiny];
					if (record.count++ == 0) {
						record.unique = warpTiny->GetUnique();
					}
				}
			}
		}

		// Update last result
		int64_t timeStamp = ITimer::GetSystemClock();
		if (timeStamp - stat->lastClockStamp > stat->statWindowDuration) {
			std::vector<std::pair<WarpTiny*, System::WarpStat::Record> > sortedResults;
			std::copy(stat->activeWarpTinies.begin(), stat->activeWarpTinies.end(), std::back_inserter(sortedResults));
			std::sort(sortedResults.begin(), sortedResults.end(), SortActiveWarpTinies);

			SpinLock(stat->critical);
			std::swap(sortedResults, stat->lastActiveWarpTinies);
			SpinUnLock(stat->critical);

			stat->activeWarpTinies.clear();
			stat->lastClockStamp = timeStamp;

			uint32_t total = 0;
			for (size_t k = 0; k < sortedResults.size(); k++) {
				total += sortedResults[k].second.count;
			}

			stat->historyOffset = (stat->historyOffset + 1) % System::WarpStat::HISTORY_LENGTH;
			float oldValue = stat->history[stat->historyOffset];
			stat->history[stat->historyOffset] = (float)total;
			stat->maxHistory = Math::Max(stat->maxHistory, (float)total);

			if (oldValue >= stat->maxHistory) {
				float newMax = 0;
				for (size_t n = 0; n < System::WarpStat::HISTORY_LENGTH; n++) {
					newMax = Math::Max(stat->history[n], newMax);
				}
				stat->maxHistory = newMax;
			}
		}
	}
};

void System::WarpStat::Execute(void* context) {
	static_cast<KernelSpy&>(kernel).Execute(this);
}

void System::WarpStat::Abort(void* context) {

}

System::WarpStat::Record::Record() : count(0) {}

void System::TickRender(LeavesFlute& leavesFlute) {
	ImGui::SetNextWindowPos(ImVec2(730, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(720, 480), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("System", &show)) {
		Kernel& kernel = leavesFlute.GetKernel();
		uint32_t warpCount = kernel.GetWarpCount();
		if (warpStats.size() != warpCount) {
			warpStats.resize(warpCount);
			for (size_t i = 0; i < warpCount; i++) {
				WarpStat* w = new WarpStat(std::ref(kernel), statWindowDuration);
				w->SetWarpIndex((uint32_t)i);
				warpStats[i] = TShared<WarpStat>::From(w);
			}
		}

		ImGui::SliderInt("Capture Window: (ms)", &statWindowDuration, 1, 5000);

		float maxHistory = 0;
		for (size_t m = 0; m < warpCount; m++) {
			TShared<WarpStat>& stat = warpStats[m];
			maxHistory = Math::Max(maxHistory, stat->maxHistory);
		}

		ImGui::Text("Warp Usage: (Max %d)", (int)maxHistory);
		ImGui::Columns(4);
		for (size_t k = 0; k < warpCount; k++) {
			TShared<WarpStat>& stat = warpStats[k];
			kernel.QueueRoutine(stat(), stat());
			char overlay[32];
			sprintf(overlay, "Warp %d", (int)k);
			ImGui::PlotLines("", stat->history, IM_ARRAYSIZE(stat->history), stat->historyOffset, overlay, 0.0f, maxHistory, ImVec2(ImGui::GetColumnWidth(), 50.0f));
			ImGui::NextColumn();
		}
		ImGui::Columns(1);

		ImGui::Text("Warp Task Ranking:");
		ImGui::Columns(3);
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "WarpIndex");
		ImGui::NextColumn();
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Object");
		ImGui::NextColumn();
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Count");
		ImGui::NextColumn();

		if (!inited) {
			ImGui::SetColumnWidth(0, 100.0f);
			ImGui::SetColumnWidth(1, 320.0f);
			ImGui::SetColumnWidth(2, 300.0f);
			inited = true;
		}

		for (size_t i = 0; i < warpCount; i++) {
			TShared<WarpStat>& stat = warpStats[i];
			std::vector<std::pair<WarpTiny*, WarpStat::Record> > sortedResults;
			SpinLock(stat->critical);
			std::swap(sortedResults, stat->lastActiveWarpTinies);
			SpinUnLock(stat->critical);

			for (size_t m = 0; m < sortedResults.size(); m++) {
				const std::pair<WarpTiny*, WarpStat::Record>& t = sortedResults[m];

				if (t.first != stat()) {
					ImGui::Text("%d", (int)i);
					ImGui::NextColumn();
					ImGui::Text("%p : %s", t.first, RemoveNamespace(t.second.unique->GetName()).c_str());
					ImGui::NextColumn();
					ImGui::Text("%d", t.second.count);
					ImGui::NextColumn();
				}
			}

			SpinLock(stat->critical);
			if (stat->lastActiveWarpTinies.empty()) {
				std::swap(sortedResults, stat->lastActiveWarpTinies);
			}
			SpinUnLock(stat->critical);
		}

		ImGui::Columns(1);
	}
	
	ImGui::End();
}