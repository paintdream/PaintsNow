// ScrypterView.cpp : implementation of the CScrypterView class
//

#include "stdafx.h"
#include "Scrypter.h"

#include "ScrypterDoc.h"
#include "ScrypterView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScrypterView

IMPLEMENT_DYNCREATE(CScrypterView, CListView)

BEGIN_MESSAGE_MAP(CScrypterView, CListView)
	//{{AFX_MSG_MAP(CScrypterView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CListView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScrypterView construction/destruction

CScrypterView::CScrypterView()
{
	// TODO: add construction code here

}

CScrypterView::~CScrypterView()
{
}

BOOL CScrypterView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterView drawing

void CScrypterView::OnDraw(CDC* pDC)
{
	CScrypterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CListCtrl& refCtrl = GetListCtrl();
	refCtrl.InsertItem(0, _T("Item!"));
	// TODO: add draw code for native data here
}

void CScrypterView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();

	CListCtrl& list = GetListCtrl();
	LONG style = ::GetWindowLong(list.m_hWnd, GWL_STYLE);
	style &= ~LVS_TYPEMASK;
	style |= LVS_REPORT;
	::SetWindowLong(list.m_hWnd, GWL_STYLE, style);

	DWORD extStyle = list.GetExtendedStyle();
	extStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	list.SetExtendedStyle(extStyle);
	list.SetTextBkColor(RGB(40, 40, 40));

	list.InsertColumn(0, _T("Index"), LVCFMT_LEFT, 50);
	list.InsertColumn(1, _T("From"), LVCFMT_LEFT, 50);
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterView printing

BOOL CScrypterView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CScrypterView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CScrypterView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterView diagnostics

#ifdef _DEBUG
void CScrypterView::AssertValid() const
{
	CListView::AssertValid();
}

void CScrypterView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CScrypterDoc* CScrypterView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CScrypterDoc)));
	return (CScrypterDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CScrypterView message handlers
void CScrypterView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}
