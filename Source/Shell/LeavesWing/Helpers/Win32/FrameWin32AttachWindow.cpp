#include "FrameWin32AttachWindow.h"
#include "../../../../Core/Template/TProxy.h"
using namespace PaintsNow;

#ifdef _WIN32

static IFrame::EventKeyboard ConvertKeyMessage(UINT nChar, UINT nFlags, int extraMask) {
	int key = 0;
	if (nChar >= VK_F1 && nChar <= VK_F12) {
		key += (nChar - VK_F1) + IFrame::EventKeyboard::KEY_F1;
	} else {
		switch (nChar) {
			case VK_PRIOR:
				key += IFrame::EventKeyboard::KEY_PAGE_UP;
				break;
			case VK_NEXT:
				key += IFrame::EventKeyboard::KEY_PAGE_DOWN;
				break;
			case VK_END:
				key += IFrame::EventKeyboard::KEY_END;
				break;
			case VK_HOME:
				key += IFrame::EventKeyboard::KEY_HOME;
				break;
			case VK_LEFT:
				key += IFrame::EventKeyboard::KEY_LEFT;
				break;
			case VK_UP:
				key += IFrame::EventKeyboard::KEY_UP;
				break;
			case VK_RIGHT:
				key += IFrame::EventKeyboard::KEY_RIGHT;
				break;
			case VK_DOWN:
				key += IFrame::EventKeyboard::KEY_DOWN;
				break;
			case VK_INSERT:
				key += IFrame::EventKeyboard::KEY_INSERT;
				break;
			case VK_DELETE:
				key += IFrame::EventKeyboard::KEY_DELETE;
				break;
			case VK_NUMLOCK:
				key += IFrame::EventKeyboard::KEY_NUM_LOCK;
				break;
			default:
				BYTE state[256];
				WORD code[2];

				if (GetKeyboardState(state)) {
					if (ToAscii(nChar, 0, state, code, 0) == 1)
						nChar = code[0] & 0xFF;
				}
				break;
		}
	}

	if (key == 0)
		key = nChar;

	if (::GetKeyState(VK_MENU) < 0)
		key |= IFrame::EventKeyboard::KEY_ALT;
	if (::GetKeyState(VK_CONTROL) < 0)
		key |= IFrame::EventKeyboard::KEY_CTRL;
	if (::GetKeyState(VK_SHIFT) < 0)
		key |= IFrame::EventKeyboard::KEY_SHIFT;

	if (nChar >= 'A' && nChar <= 'Z' && ((key & IFrame::EventKeyboard::KEY_SHIFT) == IFrame::EventKeyboard::KEY_SHIFT))
		key += 'a' - 'A';

	return IFrame::EventKeyboard(key | extraMask);
}

LRESULT CALLBACK HookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ZFrameWin32AttachWindow::ZFrameWin32AttachWindow() : window(NULL), hdc(NULL), originalProc(nullptr), callback(nullptr) {
	mainLoopEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL);
}

void ZFrameWin32AttachWindow::Attach(HWND h) {
	window = h;
}

ZFrameWin32AttachWindow::~ZFrameWin32AttachWindow() {
	::CloseHandle(mainLoopEvent);
}

void ZFrameWin32AttachWindow::SetCallback(IFrame::Callback* cb) {
	callback = cb;
}

const Int2& ZFrameWin32AttachWindow::GetWindowSize() const { return windowSize; }

void ZFrameWin32AttachWindow::SetWindowSize(const Int2& size) {
	windowSize = size;
	::SetWindowPos(window, HWND_TOP, 0, 0, size.x(), size.y(), SWP_NOMOVE);
}

void ZFrameWin32AttachWindow::SetWindowTitle(const String& title) {
	::SetWindowTextW(window, (LPCWSTR)Utf8ToSystem(title).c_str());
}

void ZFrameWin32AttachWindow::ShowCursor(CURSOR cursor) {
	if (cursor == NONE) {
		::ShowCursor(FALSE);
	} else {
		::ShowCursor(TRUE);
		LPCSTR id = IDC_ARROW;

		switch (cursor) {
			case ARROW:
				id = IDC_ARROW;
				break;
			case CROSS:
				id = IDC_CROSS;
				break;
			case WAIT:
				id = IDC_WAIT;
				break;
		}

		::SetCursor(::LoadCursor(NULL, id));
	}
}

void ZFrameWin32AttachWindow::WarpCursor(const Int2& position) {
	::SetCursorPos(position.x(), position.y());
}

void ZFrameWin32AttachWindow::EnterMainLoop() {
	hdc = ::GetWindowDC(window);
	hglrc = wglCreateContext(hdc);

	::SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)this);
	originalProc = (WNDPROC)::SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)HookProc);
	RECT rect;
	::GetWindowRect(window, &rect);
	windowSize = Int2(rect.right - rect.left, rect.bottom - rect.top);
	callback->OnWindowSize(windowSize);
	::WaitForSingleObject(mainLoopEvent, INFINITE);
	::SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)originalProc);

	wglDeleteContext(hglrc);
	::ReleaseDC(window, hdc);
}

void ZFrameWin32AttachWindow::ExitMainLoop() {
	::SetEvent(mainLoopEvent);
}

Short2 ZFrameWin32AttachWindow::MakePoint(LPARAM lParam) {
	POINT point;
	point.x = LOWORD(lParam);
	point.y = HIWORD(lParam);
	::ScreenToClient(window, &point);

	return Short2(verify_cast<short>(point.x), verify_cast<short>(point.y));
}

bool ZFrameWin32AttachWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_SIZE:
		{
			Short2 pt = MakePoint(lParam);
			callback->OnWindowSize(windowSize = Int2(pt.x(), pt.y()));
		}
		break;
		case WM_PAINT:
			wglMakeCurrent(hdc, hglrc);
			callback->OnRender();
			::InvalidateRect(hWnd, NULL, TRUE); // Prepare for next rendering
			break;
		case WM_KEYDOWN:
			callback->OnKeyboard(ConvertKeyMessage(verify_cast<UINT>(wParam), HIWORD(lParam), 0));
			break;
		case WM_KEYUP:
			callback->OnKeyboard(ConvertKeyMessage(verify_cast<UINT>(wParam), HIWORD(lParam), IFrame::EventKeyboard::KEY_POP));
			break;
		case WM_MOUSEWHEEL:
			callback->OnMouse(IFrame::EventMouse((short)HIWORD(wParam) > 0, false, false, true, MakePoint(lParam)));
			break;
		case WM_MOUSEMOVE:
			callback->OnMouse(IFrame::EventMouse(!!(wParam & MK_RBUTTON) || !!(wParam & MK_LBUTTON), true, !!(wParam & MK_RBUTTON), false, MakePoint(lParam)));
			break;
		case WM_LBUTTONDOWN:
			callback->OnMouse(IFrame::EventMouse(true, false, true, false, MakePoint(lParam)));
			break;
		case WM_LBUTTONUP:
			callback->OnMouse(IFrame::EventMouse(false, false, true, false, MakePoint(lParam)));
			break;
		case WM_RBUTTONDOWN:
			callback->OnMouse(IFrame::EventMouse(true, false, false, false, MakePoint(lParam)));
			break;
		case WM_RBUTTONUP:
			callback->OnMouse(IFrame::EventMouse(false, false, false, false, MakePoint(lParam)));
			break;
	}

	return true;
}

LRESULT CALLBACK ZFrameWin32AttachWindow::HookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ZFrameWin32AttachWindow* windowContext = reinterpret_cast<ZFrameWin32AttachWindow*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

	if (windowContext->WindowProc(hWnd, msg, wParam, lParam)) {
		return ::CallWindowProcW(windowContext->originalProc, hWnd, msg, wParam, lParam);
	} else {
		return 0;
	}
}

#endif // _WIN32
