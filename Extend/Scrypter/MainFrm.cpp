// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Scrypter.h"

#include "MainFrm.h"
#include "ScrypterDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_MESSAGE(WM_UPDATE_ALLVIEWS, OnUpdateAllViews)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

using namespace PaintsNow;
/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{

}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	HMODULE coreDll = ::LoadLibrary(_T("Shcore.dll"));
	if (coreDll != nullptr)
	{
		typedef HRESULT (WINAPI *pfnSetProcessDpiAwareness)(DWORD);
		pfnSetProcessDpiAwareness pfn = (pfnSetProcessDpiAwareness)::GetProcAddress(coreDll, "SetProcessDpiAwareness");

		if (pfn != nullptr)
		{
			pfn(2); // PROCESS_DPI_AWARENESS
		}

		::FreeLibrary(coreDll);
	}

	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	if (!m_wndDlgBar.Create(this, IDR_MAINFRAME, 
		CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}

	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndToolBar) ||
		!m_wndReBar.AddBar(&m_wndDlgBar, nullptr, nullptr, RBBS_BREAK | RBBS_GRIPPERALWAYS))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CMDIFrameWnd::PreCreateWindow(cs))
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	if (m_wndDlgBar.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return CMDIFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnActiveFrameChanged(CWnd* activeWnd)
{
	m_wndDlgBar.OnActiveFrameChanged(activeWnd);
}

CView* CMainFrame::GetMDIActiveView()
{
	CMDIChildWnd* child = MDIGetActive();
	return child == nullptr ? nullptr : child->GetActiveView();
}

CDocument* CMainFrame::GetMDIActiveDocument()
{
	CView* view = GetMDIActiveView();
	return view == nullptr ? nullptr : view->GetDocument();
}

LRESULT CMainFrame::OnUpdateAllViews(WPARAM wParam, LPARAM lParam)
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	CScrypterDoc* document = reinterpret_cast<CScrypterDoc*>(lParam);
	mainFrame->m_wndDlgBar.UpdateProgressBar(document);
	document->UpdateAllViews(nullptr, UPDATEVIEW_REFRESH);

	const TCHAR* status = _T("Ready");
	switch (document->m_document->GetStatus())
	{
	case Document::DOCUMENT_IDLE:
		status = _T("Ready");
		break;
	case Document::DOCUMENT_EXECUTING:
		status = _T("Running");
		break;
	case Document::DOCUMENT_EXTERNAL_LOCKING:
		status = _T("Busy");
		break;
	case Document::DOCUMENT_ERROR:
		status = _T("Error!");
		break;
	}

	m_wndStatusBar.SetWindowText(status);
	document->CompleteDocumentUpdated();
	return 0;
}
