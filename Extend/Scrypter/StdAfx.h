// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__8E7916A2_2E3E_4B8F_A1C6_661F5AFC78C2__INCLUDED_)
#define AFX_STDAFX_H__8E7916A2_2E3E_4B8F_A1C6_661F5AFC78C2__INCLUDED_

#define _CRT_SECURE_NO_WARNINGS
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>		// MFC socket extensions
#include "resource.h"

#include "../../Source/Core/Template/TCache.h"
#include "../../Source/Core/System/Kernel.h"
#include "../../Source/Core/Driver/Script/Lua/ZScriptLua.h"
#include "../../Source/Core/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../Source/Core/Driver/Archive/Dirent/ZArchiveDirent.h"
#include "../../Source/Core/System/StringStream.h"
#include "../../Source/Core/System/MemoryStream.h"
#include "../../Source/Core/Module/CrossScriptModule.h"
#include "../../Source/Core/Driver/Filter/Pod/ZFilterPod.h"
#include "../../Source/General/Misc/Coordinator.h"
#include "../../Source/General/Driver/Network/LibEvent/ZNetworkLibEvent.h"
#include "Executive/Worker.h"
#include "Executive/File.h"
#include "Executive/Document.h"
#include "Executive/Executive.h"
#include <vector>
#include <shlwapi.h>

#define WM_UPDATE_ALLVIEWS (WM_USER + 1001)

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__8E7916A2_2E3E_4B8F_A1C6_661F5AFC78C2__INCLUDED_)
