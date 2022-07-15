#include "ZFrameGLFW.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
#define GLFW_INCLUDE_VULKAN
#include "../../Render/Vulkan/Core/glslang/Public/ShaderLang.h"
#endif

#define GLFW_STATIC
#define GLEW_STATIC
#ifdef GLFW_LIB_PRAGMAS
#undef GLFW_LIB_PRAGMAS
#endif


#include "../../Render/OpenGL/Core/glew.h"
#include "Core/include/GLFW/glfw3.h"
#include <cassert>

using namespace PaintsNow;

static void OnErrorCallback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

void ZFrameGLFW::OnMouseButtonCallback(int button, int action, int mods) {
	double cursorX, cursorY;
	glfwGetCursorPos(window, &cursorX, &cursorY);
	IFrame::EventMouse event(lastdown = (action == GLFW_PRESS), false, lastbutton = (button == GLFW_MOUSE_BUTTON_LEFT), false, Short2((short)cursorX, (short)cursorY));

	OnMouse(event);
}

void ZFrameGLFW::OnMouseMoveCallback(double x, double y) {
	IFrame::EventMouse event(lastdown, true, lastbutton, false, Short2((short)x, (short)y));

	OnMouse(event);
}

void ZFrameGLFW::OnScrollCallback(double x, double y) {
	IFrame::EventMouse event(y > 0, false, false, true, Short2(0, 0));

	OnMouse(event);
}

void ZFrameGLFW::OnFrameBufferSizeCallback(int width, int height) {
	OnWindowSize(IFrame::EventSize(Int2(width, height)));
}

void ZFrameGLFW::OnKeyboardCallback(int key, int scancode, int action, int mods) {

	IFrame::EventKeyboard event(key > 0xff ? 0 : key);

	if (action == GLFW_RELEASE) {
		event.keyCode |= IFrame::EventKeyboard::KEY_POP;
	}

	if (key > 0xff && key <= GLFW_KEY_LAST) {
		event.keyCode |= IFrame::EventKeyboard::KEY_ESCAPE + key - GLFW_KEY_ESCAPE;
	}

	if (key & GLFW_MOD_SHIFT) {
		event.keyCode |= IFrame::EventKeyboard::KEY_SHIFT;
	}

	if (key & GLFW_MOD_CONTROL) {
		event.keyCode |= IFrame::EventKeyboard::KEY_CTRL;
	}

	if (key & GLFW_MOD_ALT) {
		event.keyCode |= IFrame::EventKeyboard::KEY_ALT;
	}

	OnKeyboard(event);
}

void OnMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->OnMouseButtonCallback(button, action, mods);
}

void OnMouseMoveCallback(GLFWwindow* window, double x, double y) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->OnMouseMoveCallback(x, y);
}

void OnScrollCallback(GLFWwindow* window, double x, double y) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->OnScrollCallback(x, y);
}

void OnFrameBufferSizeCallback(GLFWwindow* window, int width, int height) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->OnFrameBufferSizeCallback(width, height);
}

void OnKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->OnKeyboardCallback(key, scancode, action, mods);
}

ZFrameGLFW::ZFrameGLFW(GLFWwindow** windowPtr, bool vulkan, const Int2& size, IFrame::Callback* cb) : windowSize(size), lastdown(false), lastbutton(false), isVulkan(vulkan) {
	SetCallback(cb);
	glfwSetErrorCallback(OnErrorCallback);

	glfwInit();
	// We are not allowed to use BPTC in Win32 OpenGL ES context.
	/*
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	assert(glfwGetError(nullptr) == 0);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
	assert(glfwGetError(nullptr) == 0);
	*/

#ifndef _WIN32
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	if (isVulkan) {
		glfwWindowHint(GLFW_CLIENT_API, 0);
#ifdef GLFW_INCLUDE_VULKAN
		glslang::InitializeProcess();
#endif
	}

	window = glfwCreateWindow(size.x(), size.y(), "PaintsNow.Net", NULL, NULL);
	if (windowPtr != nullptr) {
		*windowPtr = window;
	}

	if (isVulkan) {
		if (!glfwVulkanSupported()) {
			fprintf(stderr, "Vulkan not supported.");
			exit(0);
		}
	}

	glfwSetWindowUserPointer(window, this);
	OnWindowSize(size);

	glfwSetKeyCallback(window, ::OnKeyboardCallback);
	glfwSetFramebufferSizeCallback(window, ::OnFrameBufferSizeCallback);
	glfwSetMouseButtonCallback(window, ::OnMouseButtonCallback);
	glfwSetCursorPosCallback(window, ::OnMouseMoveCallback);
	glfwSetScrollCallback(window, ::OnScrollCallback);

	if (!isVulkan) {
		glfwMakeContextCurrent(window);
	}
	// glfwSwapInterval(0);
}

ZFrameGLFW::~ZFrameGLFW() {
#ifdef GLFW_INCLUDE_VULKAN
	if (isVulkan) {
		glslang::FinalizeProcess();
	}
#endif

	glfwDestroyWindow(window);
	glfwTerminate();
}

void ZFrameGLFW::SetCallback(IFrame::Callback* cb) {
	callback = cb;
	if (callback != nullptr) {
		callback->OnWindowSize(windowSize);
	}
}

void ZFrameGLFW::SetWindowTitle(const String& title) {
	glfwSetWindowTitle(window, title.c_str());
}

void ZFrameGLFW::EnableVerticalSynchronization(bool enable) {
	if (!isVulkan) {
		glfwSwapInterval(enable ? 1 : 0);
	}
}

void ZFrameGLFW::OnMouse(const EventMouse& mouse) {
	if (callback != nullptr) {
		EventMouse m = mouse;
		m.position.y() = windowSize.y() - mouse.position.y();
		callback->OnMouse(m);
	}
}

void ZFrameGLFW::OnWindowSize(const EventSize& newSize) {
	windowSize = newSize.size;
	if (callback != nullptr) {
		callback->OnWindowSize(newSize);
	}
}

void ZFrameGLFW::OnKeyboard(const EventKeyboard& keyboard) {
	if (callback != nullptr) {
		callback->OnKeyboard(keyboard);
	}
}

void ZFrameGLFW::SetWindowSize(const Int2& size) {
	glfwSetWindowSize(window, size.x(), size.y());
	windowSize = size;
}

const Int2& ZFrameGLFW::GetWindowSize() const {
	return windowSize;
}

void ZFrameGLFW::OnCustomRender() {}

void ZFrameGLFW::EnterMainLoop() {
	while (!glfwWindowShouldClose(window)) {
		OPTICK_FRAME("MainThread");
		if (callback != nullptr) {
			callback->OnRender();
		}

		OnCustomRender();

		// Only OpenGL need swap buffers
		if (!isVulkan) {
			glfwSwapBuffers(window);
		}

		glfwPollEvents();
	}
}

void ZFrameGLFW::ExitMainLoop() {
	glfwSetWindowShouldClose(window, 1);
}

void ZFrameGLFW::ShowCursor(CURSOR cursor) {
	int type = GLFW_CURSOR_NORMAL;
	switch (cursor) {
		case IFrame::ARROW:
			type = GLFW_CURSOR_NORMAL;
			break;
		case IFrame::CROSS:
			type = GLFW_CURSOR_NORMAL;
			break;
		case IFrame::WAIT:
			type = GLFW_CURSOR_DISABLED;
			break;
		case IFrame::NONE:
			type = GLFW_CURSOR_HIDDEN;
			break;
	}

	glfwSetInputMode(window, GLFW_CURSOR, type);
}

void ZFrameGLFW::WarpCursor(const Int2& position) {
	glfwSetCursorPos(window, position.x(), position.y());
}

// GLFW provides release version libs ONLY!?
#if defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER > 1200
extern "C" void _except_handler4_common() {}
#endif
