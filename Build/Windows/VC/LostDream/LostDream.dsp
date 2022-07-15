# Microsoft Developer Studio Project File - Name="LostDream" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=LostDream - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LostDream.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LostDream.mak" CFG="LostDream - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LostDream - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "LostDream - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LostDream - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /Zm700 /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libc.lib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=upx                       Release\LostDream.exe                      	copy                       Release\LostDream.exe                       ..\..\..\..\Binary\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "LostDream - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MT /W3 /Zd /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /Zm600 /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcpd.lib" /nodefaultlib:"libcd.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "LostDream - Win32 Release"
# Name "LostDream - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Reflection\Annotation.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\LostDream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Spatial\Memory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Network\Network.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Reflection\NewRPC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Spatial\RandomQuery.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Reflection\Serialization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Network\ServerClient.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Network\ServerClient.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Parallel\TaskAllocator.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Reflection\Annotation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\LostDream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Spatial\Memory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Reflection\NewRPC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Parallel\Parallel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Spatial\RandomQuery.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Reflection\Reflection.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Reflection\Serialization.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Spatial\Spatial.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Shell\LostDream\Parallel\TaskAllocator.h
# End Source File
# End Group
# End Target
# End Project
