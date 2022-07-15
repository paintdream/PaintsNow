// Scrypter.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Scrypter.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "ScrypterDoc.h"
#include "LeftView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScrypterApp

BEGIN_MESSAGE_MAP(CScrypterApp, CWinApp)
	//{{AFX_MSG_MAP(CScrypterApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

using namespace PaintsNow;
/////////////////////////////////////////////////////////////////////////////
// CScrypterApp construction

CScrypterApp::CScrypterApp() 
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CScrypterApp object

CScrypterApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CScrypterApp initialization

BOOL CScrypterApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	WCHAR exeFilePath[MAX_PATH * 4];
	::GetModuleFileNameW(nullptr, exeFilePath, MAX_PATH * 4);
	m_exeFilePath = exeFilePath;
	::PathRemoveFileSpecW(exeFilePath);
	::PathAddBackslashW(exeFilePath);
	m_exeFileFolder = exeFilePath;

	SYSTEM_INFO sysinfo;
	::GetSystemInfo(&sysinfo);
	m_executive.Reset(new PaintsNow::Executive(sysinfo.dwNumberOfProcessors, SystemToUtf8(String((const char*)exeFilePath, wcslen(exeFilePath) * 2))));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#if _MSC_VER <= 1200
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_SCRYPTTYPE,
		RUNTIME_CLASS(CScrypterDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CLeftView));
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CScrypterApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

CString CScrypterApp::ConvertToRelativePath(const CString& inputPath)
{
	TCHAR target[MAX_PATH * 4];
	if (::PathRelativePathTo(target, (LPCTSTR)m_exeFileFolder, FILE_ATTRIBUTE_DIRECTORY, (LPCTSTR)inputPath, FILE_ATTRIBUTE_NORMAL))
	{
		return target;
	}
	else
	{
		return inputPath;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterApp message handlers

int CScrypterApp::ExitInstance() 
{
	m_executive.Reset((Executive*)nullptr); // in case assertion fails on root TAllocator<>
	return CWinApp::ExitInstance();
}

CString CScrypterApp::Utf8ToCString(const String& str)
{
	return (LPCWSTR)Utf8ToSystem(str).c_str();
}

String CScrypterApp::CStringToUtf8(const CString& str)
{
#ifndef _UNICODE
	DWORD dwMinSize;
	dwMinSize = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)str, (int)str.GetLength(), nullptr, 0);
	String ret;
	ret.resize((size_t)dwMinSize * sizeof(WCHAR) + sizeof(WCHAR), 0);
	::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)str, (int)str.GetLength(), (WCHAR*)(ret.data()), dwMinSize);
	return SystemToUtf8(ret);
#else
	return SystemToUtf8(String((const char*)(LPCWSTR)str, wcslen((LPCWSTR)str) * 2));
#endif
}
