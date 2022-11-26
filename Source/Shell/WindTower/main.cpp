// LostDream Test Entry
#include "../../Core/PaintsNow.h"
#include "../../General/Misc/Coordinator.h"
#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace PaintsNow;

#if defined(_MSC_VER) && _MSC_VER <= 1200 && _DEBUG
extern "C" int _CrtDbgReport() {
	return 0;
}
#endif

int main(void) {
#ifdef _WIN32
	WSADATA wsa;
	::WSAStartup(0x201, &wsa);
#endif

#ifdef _WIN32
	::WSACleanup();
#endif
	return 0;
}

