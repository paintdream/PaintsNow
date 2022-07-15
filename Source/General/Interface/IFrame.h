// IFrame.h -- Basic interface for app frame
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IDevice.h"

namespace PaintsNow {
	class pure_interface IFrame : public IDevice {
	public:
		class EventMouse {
		public:
			EventMouse(bool d = true, bool m = false, bool l = true, bool w = false, const Short2& p = Short2(0, 0), uint16_t index = 0);
			bool down;
			bool move;
			bool left;
			bool wheel;
			uint16_t index;
			Short2 position;
		};

		class EventKeyboard {
		public:
			EventKeyboard(int key = 0);
			enum
			{
				KEY_SPECIAL = 0x10000, KEY_CTRL = 0x20000, KEY_ALT = 0x40000, KEY_SHIFT = 0x80000, KEY_POP = 0x100000,
				KEY_MASK = 0xffff,
				// KEY_RIGHT_CTRL = KEY_CTRL | KEY_SELECT_RIGHT, KEY_RIGHT_ALT = KEY_ALT | KEY_SELECT_RIGHT,
				// KEY_RIGHT_SHIFT = KEY_SHIFT | KEY_SELECT_RIGHT
			};

			enum
			{
				KEY_NONE = KEY_SPECIAL,
				KEY_ESCAPE,
				KEY_ENTER,
				KEY_TAB,
				KEY_BACKSPACE,
				KEY_INSERT,
				KEY_DELETE,
				KEY_RIGHT,
				KEY_LEFT,
				KEY_DOWN,
				KEY_UP,
				KEY_PAGE_UP,
				KEY_PAGE_DOWN,
				KEY_HOME,
				KEY_END,
				KEY_CAPS_LOCK,
				KEY_SCROLL_LOCK,
				KEY_NUM_LOCK,
				KEY_PRINT_SCREEN,
				KEY_PAUSE,
				KEY_F1,
				KEY_F2,
				KEY_F3,
				KEY_F4,
				KEY_F5,
				KEY_F6,
				KEY_F7,
				KEY_F8,
				KEY_F9,
				KEY_F10,
				KEY_F11,
				KEY_F12,
				KEY_F13,
				KEY_F14,
				KEY_F15,
				KEY_F16,
				KEY_F17,
				KEY_F18,
				KEY_F19,
				KEY_F20,
				KEY_F21,
				KEY_F22,
				KEY_F23,
				KEY_F24,
				KEY_F25,
				KEY_KP_0,
				KEY_KP_1,
				KEY_KP_2,
				KEY_KP_3,
				KEY_KP_4,
				KEY_KP_5,
				KEY_KP_6,
				KEY_KP_7,
				KEY_KP_8,
				KEY_KP_9,
				KEY_KP_DECIMAL,
				KEY_KP_DIVIDE,
				KEY_KP_MULTIPLY,
				KEY_KP_SUBTRACT,
				KEY_KP_ADD,
				KEY_KP_ENTER,
				KEY_KP_EQUAL,
				KEY_LEFT_SHIFT,
				KEY_LEFT_CONTROL,
				KEY_LEFT_ALT,
				KEY_LEFT_SUPER,
				KEY_RIGHT_SHIFT,
				KEY_RIGHT_CONTROL,
				KEY_RIGHT_ALT,
				KEY_RIGHT_SUPER,
				KEY_MENU

			};

			const char* GetName() const;
			unsigned long keyCode;
		};

		class EventSize {
		public:
			EventSize(const Int2& s) : size(s) {}
			Int2 size;
		};

		class Callback {
		public:
			virtual ~Callback();
			virtual void OnMouse(const EventMouse& mouse) = 0;
			virtual void OnKeyboard(const EventKeyboard& keyboard) = 0;
			virtual void OnWindowSize(const EventSize& newSize) = 0;
			virtual void OnRender() = 0;
		};

		~IFrame() override;
		virtual void SetCallback(Callback* callback) = 0;
		virtual const Int2& GetWindowSize() const = 0;
		virtual void SetWindowSize(const Int2& size) = 0;
		virtual void SetWindowTitle(const String& title) = 0;
		virtual void EnableVerticalSynchronization(bool enable) = 0;
		enum CURSOR { NONE, ARROW, CROSS, WAIT };
		virtual void ShowCursor(CURSOR cursor) = 0;
		virtual void WarpCursor(const Int2& position) = 0;
		virtual void EnterMainLoop() = 0;
		virtual void ExitMainLoop() = 0;
	};
}

