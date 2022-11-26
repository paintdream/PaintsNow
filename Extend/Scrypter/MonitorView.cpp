// MonitorView.cpp : implementation of the CMonitorView class
//

#include "stdafx.h"
#include "Scrypter.h"

#include "ScrypterDoc.h"
#include "MonitorView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMonitorView

IMPLEMENT_DYNCREATE(CMonitorView, CListView)

BEGIN_MESSAGE_MAP(CMonitorView, CListView)
	//{{AFX_MSG_MAP(CMonitorView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CListView::OnFilePrintPreview)
END_MESSAGE_MAP()

using namespace PaintsNow;
/////////////////////////////////////////////////////////////////////////////
// CMonitorView construction/destruction

CMonitorView::CMonitorView()
{
	// TODO: add construction code here

}

CMonitorView::~CMonitorView()
{
}

BOOL CMonitorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMonitorView drawing

void CMonitorView::OnDraw(CDC* pDC)
{
	CScrypterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CListCtrl& refCtrl = GetListCtrl();
	// refCtrl.InsertItem(0, _T("Item!"));
	// TODO: add draw code for native data here
}

void CMonitorView::OnInitialUpdate()
{
	CListCtrl& list = GetListCtrl();
	LONG style = ::GetWindowLong(list.m_hWnd, GWL_STYLE);
	style &= ~LVS_TYPEMASK;
	style |= LVS_REPORT;
	::SetWindowLong(list.m_hWnd, GWL_STYLE, style);

	DWORD extStyle = list.GetExtendedStyle();
	extStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	list.SetExtendedStyle(extStyle);
	// list.SetTextBkColor(RGB(40, 40, 40));

	list.InsertColumn(0, _T("Host"), LVCFMT_LEFT, 120);
	list.InsertColumn(1, _T("Warp"), LVCFMT_LEFT, 50);
	list.InsertColumn(2, _T("Error(s)"), LVCFMT_LEFT, 60);
	list.InsertColumn(3, _T("Log"), LVCFMT_LEFT, 300);

	CListView::OnInitialUpdate();
}

/////////////////////////////////////////////////////////////////////////////
// CMonitorView printing

BOOL CMonitorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CMonitorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CMonitorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CMonitorView diagnostics

#ifdef _DEBUG
void CMonitorView::AssertValid() const
{
	CListView::AssertValid();
}

void CMonitorView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CScrypterDoc* CMonitorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CScrypterDoc)));
	return (CScrypterDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMonitorView message handlers
void CMonitorView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}

void CMonitorView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if (lHint == UPDATEVIEW_IDLE)
		return;

	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	const std::vector<RuntimeData>& runtimeData = doc->m_document->GetRuntimeData();
	CListCtrl& listCtrl = GetListCtrl();

	if (m_iterators.size() != runtimeData.size() || lHint == UPDATEVIEW_RESET)
	{
		m_iterators.clear();
		m_iterators.resize(runtimeData.size());
		for (size_t k = 0; k < runtimeData.size(); k++)
		{
			m_iterators[k] = runtimeData[k].logDataItems.begin();
		}

		listCtrl.DeleteAllItems();
		for (size_t i = 0; i < runtimeData.size(); i++)
		{
			listCtrl.InsertItem(verify_cast<int>(i), _T("localhost"));
		}
	}

	CScrypterApp* app = static_cast<CScrypterApp*>(AfxGetApp());
	const std::vector<TShared<Worker> >& workers = app->m_executive->GetWorkers();
	size_t size = Math::Min(workers.size(), runtimeData.size());

	for (size_t j = 0; j < workers.size(); j++)
	{
		CString index;
		index.Format(_T("%d"), (int)j);
		listCtrl.SetItemText((int)j, 1, index);
		CString err;
		err.Format(_T("%d"), (int)workers[j]->errorCount);
		listCtrl.SetItemText((int)j, 2, err);
	}

	for (size_t k = 0; k < runtimeData.size(); k++)
	{
		const RuntimeData& data = runtimeData[k];

		if (!data.logDataItems.Empty())
		{
			TQueueList<LogData>::const_iterator it = m_iterators[k];
			while (it != data.logDataItems.end())
			{
				m_iterators[k] = it;
				++it;
			}

			listCtrl.SetItemText((int)k, 3, CScrypterApp::Utf8ToCString(m_iterators[k]->text));
		}
		else
		{
			listCtrl.SetItemText((int)k, 3, _T(""));
		}
	}
}
