// LostDream Test Entry
#include "../../Core/PaintsNow.h"
#include "../../Utility/LeavesFlute/LeavesFlute.h"
#include "LostDream.h"
#include "Spatial/Spatial.h"
#include "Reflection/Reflection.h"
#include "Parallel/Parallel.h"
#include "Network/Network.h"
#include "../../Utility/LeavesFlute/Platform.h"
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
	LostDream lostDream;
	lostDream.RegisterQualifier(WrapFactory(UniqueType<NewRPC>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<ServerClient>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<Serialization>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<TaskAllocator>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<Memory>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<RandomQuery>()), 12);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<Annotation>()), 1);

	lostDream.RunQualifiers(true, 0, 4);
#ifdef _WIN32
	::WSACleanup();
#endif
	return 0;
}

