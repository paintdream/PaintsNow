#include "LeavesImGui.h"

#if USE_LEAVES_IMGUI

#define GLFW_STATIC
#define GLEW_STATIC
#ifdef GLFW_LIB_PRAGMAS
#undef GLFW_LIB_PRAGMAS
#endif

#include "../../../../Utility/LeavesFlute/LeavesFlute.h"
#include "Core/imgui.h"
#include "Core/imgui_impl_glfw.h"
#include "Core/imgui_impl_opengl3.h"
#include "../../../../../Driver/Render/OpenGL/Core/glew.h"
#include <GLFW/glfw3.h>

using namespace PaintsNow;

LeavesImGui::LeavesImGui(LeavesFlute*& flute) : leavesFlute(flute) {}

void LeavesImGui::AddWidget(IWidget* widget) {
	widgets.emplace_back(widget);
}

void LeavesImGui::LeaveMainLoop() {
	for (size_t i = 0; i < widgets.size(); i++) {
		widgets[i]->LeaveMainLoop();
	}
}

void LeavesImGui::TickRender() {
	if (leavesFlute != nullptr) {
		for (size_t i = 0; i < widgets.size(); i++) {
			ImGui::SetNextWindowPos(ImVec2(100, 20.0f + 50 * i), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(720, 280), ImGuiCond_FirstUseEver);
			widgets[i]->TickRender(*leavesFlute);
		}
	}
}

ZFrameGLFWImGui::ZFrameGLFWImGui(LeavesImGui& imgui, const Int2& size, Callback* callback) : ZFrameGLFW(nullptr, false, size, callback), leavesImGui(imgui), init(false) {}

void ZFrameGLFWImGui::OnMouseButtonCallback(int button, int action, int mods) {
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	if (ImGui::GetIO().WantCaptureMouse) return;
	ZFrameGLFW::OnMouseButtonCallback(button, action, mods);
}

void ZFrameGLFWImGui::OnScrollCallback(double x, double y) {
	ImGui_ImplGlfw_ScrollCallback(window, x, y);
	if (ImGui::GetIO().WantCaptureMouse) return;
	ZFrameGLFW::OnScrollCallback(x, y);
}

void ZFrameGLFWImGui::OnKeyboardCallback(int key, int scancode, int action, int mods) {
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	if (ImGui::GetIO().WantCaptureKeyboard) return;
	ZFrameGLFW::OnKeyboardCallback(key, scancode, action, mods);
}

void ZFrameGLFWImGui::EnterMainLoop() {
	ZFrameGLFW::EnterMainLoop();
	leavesImGui.LeaveMainLoop();
}

void ZFrameGLFWImGui::OnCustomRender() {
	if (!init) {
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, false);
		ImGui_ImplOpenGL3_Init();

		ImGui::StyleColorsClassic();

		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = NULL;
		io.Fonts->AddFontDefault();
		io.FontAllowUserScaling = true;

		init = true;
	}

	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	static bool showAll = false;
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsKeyPressed('`')) {
		showAll = !showAll;
	}

	if (showAll) {
		// ImGui::ShowDemoWindow();
		leavesImGui.TickRender();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#endif
