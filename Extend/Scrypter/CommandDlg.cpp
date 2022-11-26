// CommandDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Scrypter.h"
#include "CommandDlg.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "ScrypterDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCommandDlg dialog
using namespace PaintsNow;

CCommandDlg::CCommandDlg(CWnd* pParent /*=NULL*/)
	// : CDialog(CCommandDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCommandDlg)
	//}}AFX_DATA_INIT
}


void CCommandDlg::DoDataExchange(CDataExchange* pDX)
{
	// CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCommandDlg)
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Control(pDX, IDC_CONFIG_SCRIPT, m_configScript);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCommandDlg, CDialogBar)
	//{{AFX_MSG_MAP(CCommandDlg)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_GO, OnGo)
	ON_BN_CLICKED(IDC_STOP, OnStop)
	ON_BN_CLICKED(IDC_PAUSE, OnPause)
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommandDlg message handlers

LRESULT CCommandDlg::OnInitDialog(WPARAM wParam, LPARAM lParam) 
{
	// CDialogBar::OnInitDialog();
	
	LRESULT result = HandleInitDialog(wParam, lParam);
	if (!UpdateData(FALSE))
	{
		TRACE(_T("Init CCommandDlg failed!\n"));
	}

	m_progress.SetRange(0, 10000);
	return result;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
	
void CCommandDlg::OnActiveFrameChanged(CWnd* activeWnd)
{
	CChildFrame* frame = static_cast<CChildFrame*>(activeWnd);
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(frame->GetActiveDocument());

	if (doc != nullptr)
	{
		m_configScript.SetWindowText(CScrypterApp::Utf8ToCString(doc->m_document->GetPluginPath()));
		UpdateProgressBar(doc);
	}
}

void CCommandDlg::UpdateProgressBar(CScrypterDoc* doc)
{
	m_progress.SetPos((int)(doc->m_document->GetProfile().ratio * 10000));
}

void CCommandDlg::OnBrowse() 
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(mainFrame->GetMDIActiveDocument());
	if (doc != nullptr)
	{
		Document* document = doc->m_document();
		if (document->AcquireLock())
		{
			CFileDialog file(TRUE, nullptr, nullptr, 0, _T("Scrypter script (*.lua)|*.lua||"));
			CScrypterApp* app = static_cast<CScrypterApp*>(AfxGetApp());
			file.m_ofn.lpstrInitialDir = app->m_exeFileFolder;

			bool newScript = file.DoModal() == IDOK;
			if (newScript)
			{
				CString path = app->ConvertToRelativePath(file.GetPathName());
				m_configScript.SetWindowText(path);
				document->SetPluginPath(CScrypterApp::CStringToUtf8(path));
			}

			document->ReleaseLock();

			if (newScript)
			{
				RefreshDocument();
			}
		}
		else
		{
			MessageBox(_T("Document is busy."));
		}
	}
	else
	{
		MessageBox(_T("No active document present, please create one."));
	}
}

void CCommandDlg::RefreshDocument()
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(mainFrame->GetMDIActiveDocument());
	if (doc != nullptr)
	{
		doc->m_document->InvokeReload();
	}
}

void CCommandDlg::OnGo() 
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(mainFrame->GetMDIActiveDocument());
	if (doc != nullptr)
	{
		doc->Clear();	
		doc->m_document->InvokeMain();
	}
}

void CCommandDlg::OnStop() 
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(mainFrame->GetMDIActiveDocument());
	if (doc != nullptr)
	{
		// not implemented
	}
}

void CCommandDlg::OnPause() 
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(mainFrame->GetMDIActiveDocument());
	if (doc != nullptr)
	{
		// not implemented
	}
}

void CCommandDlg::OnClear() 
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(mainFrame->GetMDIActiveDocument());
	if (doc != nullptr)
	{
		Document* document = doc->m_document();
		document->ClearError();

		if (document->AcquireLock())
		{
			document->Clear();
			document->ReleaseLock();
		}
	}
}
