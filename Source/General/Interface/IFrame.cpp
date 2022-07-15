#include "IFrame.h"

using namespace PaintsNow;

IFrame::EventMouse::EventMouse(bool d, bool m, bool l, bool w, const Short2& p, uint16_t i) : down(d), move(m), left(l), wheel(w), position(p), index(i) {}

IFrame::EventKeyboard::EventKeyboard(int key) : keyCode(key) {}

const char* IFrame::EventKeyboard::GetName() const {
	long key = keyCode;
	if (!!(key & KEY_SPECIAL)) {
		static const char* specialMap[] = {
			"NULL", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "LEFT", "UP", "RIGHT", "DOWN", "PAGEUP", "PAGEDOWN", "HOME", "END", "INSERT", "DELETE", "NUM_LOCK", "BEGIN"
		};

		return specialMap[(key & KEY_MASK) % (sizeof(specialMap) / sizeof(const char*))];
	} else {
		switch (key & ~KEY_POP) {
			case KEY_CTRL:
				return "CTRL";
			case KEY_ALT:
				return "ALT";
			case KEY_SHIFT:
				return "SHIFT";
		}

		static const char* charMap[] = {
			"NULL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
			"BACKSPACE", "HT", "LF", "VT", "FF", "RETURN", "SO", "SI",
			"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
			"CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US",
			" ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*",
			"+", ",", "-", ".", "/", "0", "1", "2", "3", "4", "5",
			"6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
			"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
			"L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V",
			"W", "X", "Y", "Z", "[", "\\", "]", "^", "_", "`", "a",
			"b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l",
			"m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w",
			"x", "y", "z", "{", "|", "|", "~", "DELETE"
		};

		return charMap[(key & KEY_MASK) % (sizeof(charMap) / sizeof(const char*))];
	}
}

IFrame::Callback::~Callback() {}
IFrame::~IFrame() {}