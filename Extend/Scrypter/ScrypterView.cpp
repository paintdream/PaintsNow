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
	ON_NOTIFY_REFLECT(LVN_ODCACHEHINT, OnOdcachehint)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM, OnOdfinditem)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CListView::OnFilePrintPreview)
END_MESSAGE_MAP()

using namespace PaintsNow;
/////////////////////////////////////////////////////////////////////////////
// CScrypterView construction/destruction

CScrypterView::CScrypterView() : m_lastFilteredRecordIterator(m_filteredRecordData.begin()), m_lastFilteredRecordIndex(~(size_t)0 - 1u), m_filteredRecordCount(0)
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
	cs.style |= LVS_OWNERDATA;
	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterView drawing

void CScrypterView::OnDraw(CDC* pDC)
{
	CScrypterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// CListCtrl& refCtrl = GetListCtrl();
	// refCtrl.InsertItem(0, _T("Item!"));
	// TODO: add draw code for native data here
}

void CScrypterView::OnInitialUpdate()
{
	CListCtrl& list = GetListCtrl();
	LONG style = ::GetWindowLong(list.m_hWnd, GWL_STYLE);
	style &= ~LVS_TYPEMASK;
	style |= LVS_REPORT;
	::SetWindowLong(list.m_hWnd, GWL_STYLE, style);

	DWORD extStyle = list.GetExtendedStyle();
	extStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES; // use virtual list
	list.SetExtendedStyle(extStyle);
	// list.SetTextBkColor(RGB(40, 40, 40));

	list.InsertColumn(0, _T("Index"), LVCFMT_LEFT, 50);
	list.InsertColumn(1, _T("Object"), LVCFMT_LEFT, 320);
	list.InsertColumn(2, _T("Description"), LVCFMT_LEFT, 450);

	CListView::OnInitialUpdate();
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

void CScrypterView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CListCtrl& listCtrl = GetListCtrl();
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	const std::vector<RuntimeData>& runtimeData = doc->m_document->GetRuntimeData();

	if (m_iterators.size() != runtimeData.size() || lHint == UPDATEVIEW_RESET)
	{
		m_iterators.clear();
		m_iterators.resize(runtimeData.size());
		for (size_t k = 0; k < runtimeData.size(); k++)
		{
			m_iterators[k] = runtimeData[k].recordDataItems.begin();
		}

		m_filteredRecordData.Clear();
		m_lastFilteredRecordIterator = m_filteredRecordData.begin();
		m_lastFilteredRecordIndex = ~(size_t)0 - 1u;
		m_filteredRecordCount = 0;
		listCtrl.SetItemCountEx(0, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
	}

	if (lHint == UPDATEVIEW_IDLE)
	{
		CScrypterApp* app = static_cast<CScrypterApp*>(AfxGetApp());
		app->BeginWaitCursor();

		const DWORD MAX_TICKS = 8;
		const size_t CHECK_COUNT = 256;
		DWORD tickCount = ::GetTickCount();

		size_t newRecordCount = 0;
		size_t totalCount = 0;

		bool breakOut = false;
		for (size_t i = 0; i < runtimeData.size() && !breakOut; i++)
		{
			const RuntimeData& data = runtimeData[i];
			TQueueList<RecordData>::const_iterator it = m_iterators[i];
			while (it != data.recordDataItems.end())
			{
				if (FilterRecord(*it))
				{
					const RecordData* p = &*it;
					m_filteredRecordData.Push(p);
					newRecordCount++;
				}

				++it;

				if (++totalCount % CHECK_COUNT == 0)
				{
					if (::GetTickCount() - tickCount >= MAX_TICKS)
					{
						breakOut = true;
						break;
					}
				}
			}

			m_iterators[i] = it;
		}

		if (newRecordCount != 0)
			listCtrl.SetItemCountEx((int)(m_filteredRecordCount += newRecordCount), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);

		app->EndWaitCursor();
	}
	else if (lHint == UPDATEVIEW_REDRAW || lHint == UPDATEVIEW_RESET)
	{
		listCtrl.RedrawItems(0, (int)m_filteredRecordCount);
	}
}

inline bool CScrypterView::FilterRecord(const PaintsNow::RecordData& record)
{
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	size_t n = record.object.size();
	size_t k = doc->m_filterPrefix.size();
	if (k == 0)
	{
		return true;
	}
	else if (n >= k)
	{
		return memcmp(doc->m_filterPrefix.data(), record.object.data(), k) == 0;
	}
	else
	{
		return false;
	}
}

inline const RecordData* CScrypterView::FindRecord(size_t index)
{
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	if (index >= m_filteredRecordCount)
	{
		m_lastFilteredRecordIndex = ~(size_t)0 - 1u;
		return nullptr;
	}

	if (index == m_lastFilteredRecordIndex + 1)
	{
		++m_lastFilteredRecordIterator;
	}
	else
	{
		m_lastFilteredRecordIterator = m_filteredRecordData.begin() + verify_cast<uint32_t>(index);
	}

	m_lastFilteredRecordIndex = index;
	return *m_lastFilteredRecordIterator;
}

void CScrypterView::OnOdcachehint(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVCACHEHINT* pCacheHint = (NMLVCACHEHINT*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CScrypterView::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const RecordData* recordData = FindRecord(pItem->iItem);
		if (recordData != nullptr)
		{
			switch (pItem->iSubItem)
			{
			case 0:
				_stprintf(pItem->pszText, _T("%d"), pItem->iItem);
				break;
			case 1:
				lstrcpyn(pItem->pszText, (LPCTSTR)CScrypterApp::Utf8ToCString(recordData->object), pItem->cchTextMax);
				break;
			case 2:
				lstrcpyn(pItem->pszText, (LPCTSTR)CScrypterApp::Utf8ToCString(recordData->description), pItem->cchTextMax);
				break;
			}
		}
	}

	*pResult = 0;
}

void CScrypterView::OnOdfinditem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVFINDITEM* pFindInfo = (NMLVFINDITEM*)pNMHDR;
	CListCtrl& listCtrl = GetListCtrl();
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

static const UINT MENU_COMMAND_BASE = WM_USER + 0x1000;

void CScrypterView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	Document* document = static_cast<CScrypterDoc*>(GetDocument())->m_document();
	if (document->AcquireLock())
	{
		const std::vector<String>& operations = document->GetOperations();
		CMenu menu;
		menu.CreatePopupMenu();
		for (size_t i = 0; i < operations.size(); i++)
		{
			menu.AppendMenu(MF_STRING, (UINT_PTR)(MENU_COMMAND_BASE + i), CScrypterApp::Utf8ToCString(operations[i]));
		}

		CPoint pt;
		::GetCursorPos(&pt);
		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
		document->ReleaseLock();
	}

	*pResult = 0;
}

void CScrypterView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnCommand(MENU_COMMAND_BASE, 0);
	*pResult = 0;
}

BOOL CScrypterView::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	UINT command = LOWORD(wParam);
	if (command >= MENU_COMMAND_BASE)
	{
		CListCtrl& listCtrl = GetListCtrl();
		POSITION pos = listCtrl.GetFirstSelectedItemPosition();
		std::vector<const RecordData*> records;
		records.reserve(listCtrl.GetSelectedCount());

		while (pos != nullptr)
		{
			int item = listCtrl.GetNextSelectedItem(pos);
			const RecordData* d = FindRecord(item);
			if (d != nullptr)
			{
				records.emplace_back(d);
			}
		}

		Document* document = static_cast<CScrypterDoc*>(GetDocument())->m_document();
		document->InvokeOperation(command - MENU_COMMAND_BASE, std::move(records));
	}

	return CListView::OnCommand(wParam, lParam);
}
