# Microsoft Developer Studio Project File - Name="Scrypter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Scrypter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Scrypter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Scrypter.mak" CFG="Scrypter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Scrypter - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Scrypter - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Scrypter - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D _WIN32_WINNT=0x501 /D "USE_RAW_MEMORY_ALIGNMENT" /Yu"stdafx.h" /FD /Zm800 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 uafxcw.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy    Release\Scrypter.exe    ..\..\Binary\	upx --best --8mib-ram --no-reloc --ultra-brute ..\..\Binary\Scrypter.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Scrypter - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D _WIN32_WINNT=0x501 /D "USE_RAW_MEMORY_ALIGNMENT" /Yu"stdafx.h" /FD /GZ /Zm800 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 uafxcwd.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Scrypter - Win32 Release"
# Name "Scrypter - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\CommandDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LeftView.cpp
# End Source File
# Begin Source File

SOURCE=.\LogView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MonitorView.cpp
# End Source File
# Begin Source File

SOURCE=.\ParameterView.cpp
# End Source File
# Begin Source File

SOURCE=.\Scrypter.cpp
# End Source File
# Begin Source File

SOURCE=.\Scrypter.rc
# End Source File
# Begin Source File

SOURCE=.\ScrypterDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrypterView.cpp
# End Source File
# Begin Source File

SOURCE=.\ServiceDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\CommandDlg.h
# End Source File
# Begin Source File

SOURCE=.\LeftView.h
# End Source File
# Begin Source File

SOURCE=.\LogView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MonitorView.h
# End Source File
# Begin Source File

SOURCE=.\ParameterView.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Scrypter.h
# End Source File
# Begin Source File

SOURCE=.\ScrypterDoc.h
# End Source File
# Begin Source File

SOURCE=.\ScrypterView.h
# End Source File
# Begin Source File

SOURCE=.\ServiceDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Scrypter.ico
# End Source File
# Begin Source File

SOURCE=.\res\Scrypter.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ScrypterDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Group "Core Files"

# PROP Default_Filter ""
# Begin Group "Backport"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Backport\VC98STRING.h
# End Source File
# End Group
# Begin Group "Driver"

# PROP Default_Filter ""
# Begin Group "Archive"

# PROP Default_Filter ""
# Begin Group "Dirent"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Archive\Dirent\ZArchiveDirent.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Archive\Dirent\ZArchiveDirent.h
# End Source File
# End Group
# End Group
# Begin Group "Filter"

# PROP Default_Filter ""
# Begin Group "Pod"

# PROP Default_Filter ""
# Begin Group "Pod Core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Filter\Pod\Core\Pod.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Filter\Pod\Core\Pod.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Filter\Pod\ZFilterPod.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Filter\Pod\ZFilterPod.h
# End Source File
# End Group
# End Group
# Begin Group "Script"

# PROP Default_Filter ""
# Begin Group "Lua"

# PROP Default_Filter ""
# Begin Group "Lua Core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lapi.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lapi.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lauxlib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lauxlib.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lbaselib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lcode.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lcode.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lcorolib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lctype.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lctype.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ldblib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ldebug.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ldebug.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ldo.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ldo.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ldump.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lfunc.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lfunc.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lgc.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lgc.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\linit.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\liolib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ljumptab.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\llex.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\llex.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\llimits.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lmathlib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lmem.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lmem.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\loadlib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lobject.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lobject.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lopcodes.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lopcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lopnames.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\loslib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lparser.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lparser.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lprefix.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lstate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lstate.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lstring.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lstring.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lstrlib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ltable.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ltable.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ltablib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ltm.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\ltm.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lua.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\luaconf.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lualib.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lundump.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lundump.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lutf8lib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lvm.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lvm.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lzio.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\Core\lzio.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\ZScriptLua.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Script\Lua\ZScriptLua.h
# End Source File
# End Group
# End Group
# Begin Group "Thread"

# PROP Default_Filter ""
# Begin Group "Pthread"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Thread\Pthread\ZThreadPthread.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Driver\Thread\Pthread\ZThreadPthread.h
# End Source File
# End Group
# End Group
# Begin Group "Network"

# PROP Default_Filter ""
# Begin Group "LibEvent"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\General\Driver\Network\LibEvent\ZNetworkLibEvent.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Driver\Network\LibEvent\ZNetworkLibEvent.h
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IArchive.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IArchive.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IDevice.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IDevice.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IFilterBase.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IFilterBase.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IMemory.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Interface\INetwork.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Interface\INetwork.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IReflect.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IReflect.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IScript.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IScript.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IStreamBase.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IStreamBase.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\ITask.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\ITask.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IThread.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IThread.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Interface\ITunnel.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Interface\ITunnel.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IType.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Interface\IType.h
# End Source File
# End Group
# Begin Group "System"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\System\ConsoleStream.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\ConsoleStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Graph.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Graph.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Kernel.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Kernel.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\MemoryStream.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\MemoryStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Quota.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Quota.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\ShadowStream.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\ShadowStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\StringStream.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\StringStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Tape.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Tape.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\TaskGraph.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\TaskGraph.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\TaskQueue.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\TaskQueue.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\ThreadPool.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\ThreadPool.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Tiny.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\System\Tiny.h
# End Source File
# End Group
# Begin Group "Template"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Template\TAlgorithm.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TAllocator.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TAtomic.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TBuffer.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TCache.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TContainer.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TEvent.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TKdTree.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TMap.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TMatrix.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TObject.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TPool.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TProxy.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TQueue.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TTagged.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Template\TVector.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Module"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\Core\Module\CrossScript.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Module\CrossScript.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Module\CrossScriptModule.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\Module\CrossScriptModule.h
# End Source File
# End Group
# Begin Group "Misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Source\General\Misc\Coordinator.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Misc\Coordinator.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Misc\RemoteCall.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\General\Misc\RemoteCall.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Source\Core\PaintsNow.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Source\Core\PaintsNow.h
# End Source File
# End Group
# Begin Group "Executive Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Executive\Document.cpp
# End Source File
# Begin Source File

SOURCE=.\Executive\Document.h
# End Source File
# Begin Source File

SOURCE=.\Executive\Executive.cpp
# End Source File
# Begin Source File

SOURCE=.\Executive\Executive.h
# End Source File
# Begin Source File

SOURCE=.\Executive\File.cpp
# End Source File
# Begin Source File

SOURCE=.\Executive\File.h
# End Source File
# Begin Source File

SOURCE=.\Executive\Worker.cpp
# End Source File
# Begin Source File

SOURCE=.\Executive\Worker.h
# End Source File
# End Group
# Begin Group "Script Files"

# PROP Default_Filter "lua;luac"
# Begin Source File

SOURCE=.\res\Core.lua
# End Source File
# Begin Source File

SOURCE=.\script\datahunter.lua
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\XPStyle.manifest
# End Source File
# End Target
# End Project
