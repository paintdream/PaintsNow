#include "IModule.h"
#include "../../../../Utility/LeavesFlute/LeavesFlute.h"
#include "Core/imgui.h"
#include <sstream>

using namespace PaintsNow;

void IModule::TickRender(LeavesFlute& leavesFlute) {
	ImGui::SetNextWindowPos(ImVec2(730, 480), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(720, 320), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Module", &show)) {
		RenderObject(leavesFlute);
	}

	ImGui::End();
}
