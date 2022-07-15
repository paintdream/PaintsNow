// ZFrameGLFW.h -- App frame based on GLFW
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once
#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IFrame.h"
#include <cstdlib>

struct GLFWwindow;
namespace PaintsNow {
	class ZFrameGLFW : public IFrame {
	public:
		ZFrameGLFW(GLFWwindow** windowPtr, bool isVulkan = false, const Int2& size = Int2(960, 600), Callback* callback = nullptr);
		~ZFrameGLFW() override;
		void SetCallback(Callback* callback) override;
		const Int2& GetWindowSize() const override;
		void SetWindowSize(const Int2& size) override;
		void SetWindowTitle(const String& title) override;
		void EnableVerticalSynchronization(bool enable) override;

		void EnterMainLoop() override;
		void ExitMainLoop() override;

		void OnMouse(const EventMouse& mouse);
		void OnKeyboard(const EventKeyboard& keyboard);
		void OnWindowSize(const EventSize& newSize);
		void ShowCursor(CURSOR cursor) override;
		void WarpCursor(const Int2& position) override;

	public:
		virtual void OnMouseButtonCallback(int button, int action, int mods);
		virtual void OnMouseMoveCallback(double x, double y);
		virtual void OnScrollCallback(double x, double y);
		virtual void OnFrameBufferSizeCallback(int width, int height);
		virtual void OnKeyboardCallback(int key, int scancode, int action, int mods);
		virtual void OnCustomRender();

	protected:
		GLFWwindow* window;
		Callback* callback;
		Int2 windowSize;
		bool isVulkan;
		bool lastbutton;
		bool lastdown;
		bool reserved;
	};
}

