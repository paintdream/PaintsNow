// FrameWin32AttachWindow.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-28
//

#pragma once

#ifdef _WIN32

#include "../../../../General/Interface/IFrame.h"
#include <windows.h>

namespace PaintsNow {
	class ZFrameWin32AttachWindow : public IFrame {
	public:
		ZFrameWin32AttachWindow();
		void Attach(HWND h);
		~ZFrameWin32AttachWindow() override;

	protected:
		void SetCallback(IFrame::Callback* cb) override;
		const Int2& GetWindowSize() const override;
		void SetWindowSize(const Int2& size) override;
		void SetWindowTitle(const String& title) override;
		void ShowCursor(CURSOR cursor) override;
		void WarpCursor(const Int2& position) override;
		void EnterMainLoop() override;
		void ExitMainLoop() override;
		Short2 MakePoint(LPARAM lParam);
		bool WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK HookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	protected:
		HWND window;
		HDC hdc;
		HGLRC hglrc;
		WNDPROC originalProc;
		Callback* callback;
		HANDLE mainLoopEvent;
		Int2 windowSize;
	};
}

#endif