; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CScrypterApp
LastTemplate=CListView
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "Scrypter.h"
LastPage=0

ClassCount=11
Class1=CScrypterApp
Class2=CScrypterDoc
Class3=CScrypterView
Class4=CMainFrame
Class7=CAboutDlg

ResourceCount=7
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDR_SCRYPTTYPE
Class5=CChildFrame
Class6=CLeftView
Resource4=IDD_ABOUTBOX (English (U.S.))
Resource5=IDR_SCRYPTTYPE (English (U.S.))
Class8=CMonitorView
Class9=CLogView
Class10=CCommandDlg
Class11=CParameterView
Resource6=IDR_MAINFRAME (English (U.S.))
Resource7=IDD_SERVICE (English (U.S.))

[CLS:CScrypterApp]
Type=0
HeaderFile=Scrypter.h
ImplementationFile=Scrypter.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC
LastObject=CScrypterApp

[CLS:CScrypterDoc]
Type=0
HeaderFile=ScrypterDoc.h
ImplementationFile=ScrypterDoc.cpp
Filter=N
LastObject=CScrypterDoc
BaseClass=CDocument
VirtualFilter=DC

[CLS:CScrypterView]
Type=0
HeaderFile=ScrypterView.h
ImplementationFile=ScrypterView.cpp
Filter=C
LastObject=CScrypterView
BaseClass=CListView
VirtualFilter=VWC


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=IDC_CONFIG_SERVICE
BaseClass=CMDIFrameWnd
VirtualFilter=fWC


[CLS:CChildFrame]
Type=0
HeaderFile=ChildFrm.h
ImplementationFile=ChildFrm.cpp
Filter=M
LastObject=IDC_CONFIG_SCRIPT
BaseClass=CMDIChildWnd
VirtualFilter=mfWC

[CLS:CLeftView]
Type=0
HeaderFile=LeftView.h
ImplementationFile=LeftView.cpp
Filter=T
BaseClass=CTreeView
VirtualFilter=VWC
LastObject=CLeftView

[CLS:CAboutDlg]
Type=0
HeaderFile=Scrypter.cpp
ImplementationFile=Scrypter.cpp
Filter=D
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Class=CAboutDlg

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_PRINT_SETUP
Command4=ID_FILE_MRU_FILE1
Command5=ID_APP_EXIT
Command6=ID_VIEW_TOOLBAR
Command7=ID_VIEW_STATUS_BAR
CommandCount=8
Command8=ID_APP_ABOUT

[TB:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
CommandCount=8
Command8=ID_APP_ABOUT

[MNU:IDR_SCRYPTTYPE]
Type=1
Class=CScrypterView
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_CLOSE
Command4=ID_FILE_SAVE
Command5=ID_FILE_SAVE_AS
Command6=ID_FILE_PRINT
Command7=ID_FILE_PRINT_PREVIEW
Command8=ID_FILE_PRINT_SETUP
Command9=ID_FILE_MRU_FILE1
Command10=ID_APP_EXIT
Command11=ID_EDIT_UNDO
Command12=ID_EDIT_CUT
Command13=ID_EDIT_COPY
Command14=ID_EDIT_PASTE
CommandCount=21
Command15=ID_VIEW_TOOLBAR
Command16=ID_VIEW_STATUS_BAR
Command17=ID_WINDOW_NEW
Command18=ID_WINDOW_CASCADE
Command19=ID_WINDOW_TILE_HORZ
Command20=ID_WINDOW_ARRANGE
Command21=ID_APP_ABOUT

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
CommandCount=14
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE


[DLG:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
ControlCount=9
Control1=IDC_PROGRESS,msctls_progress32,1350565888
Control2=IDC_STATIC,static,1342308865
Control3=IDC_CONFIG_SCRIPT,edit,1350633600
Control4=IDC_BROWSE,button,1342242816
Control5=IDC_GO,button,1342242817
Control6=IDC_PAUSE,button,1342242816
Control7=IDC_STOP,button,1342242816
Control8=IDC_EXPORT,button,1342242816
Control9=IDC_CLEAR,button,1342242816

[TB:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_VIEW_LARGEICON
Command9=ID_VIEW_SMALLICON
Command10=ID_VIEW_LIST
Command11=ID_VIEW_DETAILS
Command12=ID_APP_ABOUT
CommandCount=12

[MNU:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_PRINT_SETUP
Command4=ID_FILE_MRU_FILE1
Command5=ID_APP_EXIT
Command6=ID_VIEW_TOOLBAR
Command7=ID_VIEW_STATUS_BAR
Command8=ID_WINDOW_SPLIT
Command9=ID_VIEW_LARGEICON
Command10=ID_VIEW_SMALLICON
Command11=ID_VIEW_LIST
Command12=ID_VIEW_DETAILS
Command13=ID_VIEW_BYNAME
Command14=ID_VIEW_AUTOARRANGE
Command15=ID_VIEW_LINEUP
Command16=IDC_CONFIG_SERVICE
Command17=ID_APP_ABOUT
CommandCount=17

[MNU:IDR_SCRYPTTYPE (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_CLOSE
Command4=ID_FILE_SAVE
Command5=ID_FILE_SAVE_AS
Command6=ID_FILE_PRINT
Command7=ID_FILE_PRINT_PREVIEW
Command8=ID_FILE_PRINT_SETUP
Command9=ID_FILE_MRU_FILE1
Command10=ID_APP_EXIT
Command11=ID_EDIT_UNDO
Command12=ID_EDIT_CUT
Command13=ID_EDIT_COPY
Command14=ID_EDIT_PASTE
Command15=ID_VIEW_TOOLBAR
Command16=ID_VIEW_STATUS_BAR
Command17=ID_WINDOW_SPLIT
Command18=ID_VIEW_LARGEICON
Command19=ID_VIEW_SMALLICON
Command20=ID_VIEW_LIST
Command21=ID_VIEW_DETAILS
Command22=ID_VIEW_BYNAME
Command23=ID_VIEW_AUTOARRANGE
Command24=ID_VIEW_LINEUP
Command25=ID_WINDOW_NEW
Command26=ID_WINDOW_CASCADE
Command27=ID_WINDOW_TILE_HORZ
Command28=ID_WINDOW_ARRANGE
Command29=ID_APP_ABOUT
CommandCount=29

[ACL:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE
CommandCount=14

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[CLS:CMonitorView]
Type=0
HeaderFile=MonitorView.h
ImplementationFile=MonitorView.cpp
BaseClass=CListView
Filter=C
LastObject=CMonitorView
VirtualFilter=VWC

[CLS:CLogView]
Type=0
HeaderFile=LogView.h
ImplementationFile=LogView.cpp
BaseClass=CEditView
Filter=C
LastObject=CLogView
VirtualFilter=VWC

[CLS:CCommandDlg]
Type=0
HeaderFile=CommandDlg.h
ImplementationFile=CommandDlg.cpp
BaseClass=CDialogBar
Filter=D
LastObject=CCommandDlg
VirtualFilter=dWC

[CLS:CParameterView]
Type=0
HeaderFile=ParameterView.h
ImplementationFile=ParameterView.cpp
BaseClass=CListView
Filter=C
LastObject=CParameterView
VirtualFilter=VWC

[DLG:IDD_SERVICE (English (U.S.))]
Type=1
Class=?
ControlCount=9
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,button,1342177287
Control4=IDC_AS_WINDOWS_SERVICE,button,1342242819
Control5=IDC_ACCEPT_REQUESTS,button,1342242819
Control6=IDC_STATIC,static,1342308352
Control7=IDC_UPSTREAM_ADDRESS,edit,1350631552
Control8=IDC_STATIC,static,1342308352
Control9=IDC_MAX_THREADS,edit,1350631552

