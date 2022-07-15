// Platform.h
// PaintDream (paintdream@paintdream.com)
// 2018-8-24
//

#pragma once
#include "../../Core/PaintsNow.h"

#define PAINTSNOW_VERSION_MAJOR "0"
#define PAINTSNOW_VERSION_MINOR "0.22.7.16"

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif // _MSC_VER

#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib, "libmp3lame.lib")
#pragma comment(lib, "mpghip.lib")
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib, "glew32s.lib")
#endif

#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glslang.lib")
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib, "OpenAL32.lib")
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "WinMM.lib")
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib, "freetype.lib")
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib, "freeimage.lib")
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib, "glfw3.lib")
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "OpenGL32.lib")
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib, "libevent.lib")
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#ifdef _DEBUG
#pragma comment(lib, "../../../../Build/Windows/VC/PaintsNow/Debug/PaintsNow.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/HeartVioliner/Debug/HeartVioliner.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/SnowyStream/Debug/SnowyStream.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/MythForest/Debug/MythForest.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/BridgeSunset/Debug/BridgeSunset.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/LeavesFlute/Debug/LeavesFlute.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/EchoLegend/Debug/EchoLegend.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/Remembery/Debug/Remembery.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/GalaxyWeaver/Debug/GalaxyWeaver.lib")
#else
#pragma comment(lib, "../../../../Build/Windows/VC/PaintsNow/Release/PaintsNow.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/HeartVioliner/Release/HeartVioliner.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/SnowyStream/Release/SnowyStream.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/MythForest/Release/MythForest.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/BridgeSunset/Release/BridgeSunset.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/LeavesFlute/Release/LeavesFlute.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/EchoLegend/Release/EchoLegend.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/Remembery/Release/Remembery.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/GalaxyWeaver/Release/GalaxyWeaver.lib")
#endif
#endif // _MSC_VER

