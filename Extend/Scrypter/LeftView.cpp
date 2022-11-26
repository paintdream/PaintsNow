// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "Scrypter.h"

#include "ScrypterDoc.h"
#include "LeftView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	//{{AFX_MSG_MAP(CLeftView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

using namespace PaintsNow;
/////////////////////////////////////////////////////////////////////////////
// CLeftView construction/destruction

CLeftView::CLeftView() : m_rootTreeItem(TVI_ROOT)
{
	// TODO: add construction code here

}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView drawing

void CLeftView::OnDraw(CDC* pDC)
{
	CScrypterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}


/////////////////////////////////////////////////////////////////////////////
// CLeftView printing

BOOL CLeftView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CLeftView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CLeftView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CLeftView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	// TODO: You may populate your TreeView with items by directly accessing
	//  its tree control through a call to GetTreeCtrl().
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CScrypterDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CScrypterDoc)));
	return (CScrypterDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftView message handlers

HTREEITEM CLeftView::MakeItemRecursive(StringView& object, bool addItem)
{
	for (size_t i = object.size(); i != 0; i--)
	{
		char ch = object.data()[i - 1];
		if (ch == '/' || ch == '\\') // split path
		{
			StringView route(object.data(), i - 1);
			std::unordered_map<StringView, HTREEITEM>::iterator it = m_mapPathToTreeItem.find(route);
			object = StringView(object.data() + i, object.size() - i);

			if (it == m_mapPathToTreeItem.end())
			{
				StringView name = route;
				HTREEITEM parent = MakeItemRecursive(name, true);
				if (addItem)
				{
					return m_mapPathToTreeItem[route] = GetTreeCtrl().InsertItem(TVIF_TEXT | TVIF_PARAM, CScrypterApp::Utf8ToCString(name), 0, 0, 0, 0, ch, parent, TVI_LAST);
				}
				else
				{
					return m_rootTreeItem;
				}
			}
			else
			{
				return (*it).second;
			}
		}
	}

	return m_rootTreeItem;
}

void CLeftView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == UPDATEVIEW_IDLE || pSender == this)
		return;

	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	const std::vector<RuntimeData>& runtimeData = doc->m_document->GetRuntimeData();
	CTreeCtrl& tree = GetTreeCtrl();

	if (m_iterators.size() != runtimeData.size() || lHint == UPDATEVIEW_RESET)
	{
		m_iterators.clear();
		m_iterators.resize(runtimeData.size());
		for (size_t k = 0; k < runtimeData.size(); k++)
		{
			m_iterators[k] = runtimeData[k].recordDataItems.begin();
		}

		tree.DeleteAllItems();
		m_mapPathToTreeItem.clear();
		m_rootTreeItem = tree.InsertItem(_T("/"));
		tree.Expand(m_rootTreeItem, TVE_EXPAND);
		doc->m_filterPrefix = "";
	}

	for (size_t i = 0; i < runtimeData.size(); i++)
	{
		const RuntimeData& data = runtimeData[i];
		TQueueList<RecordData>::const_iterator it = m_iterators[i];
		while (it != data.recordDataItems.end())
		{
			StringView object = it->object;
			MakeItemRecursive(object, false);
			++it;
		}

		m_iterators[i] = it;
	}

	tree.Expand(m_rootTreeItem, TVE_EXPAND);
}

void CLeftView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM item = pNMTreeView->itemNew.hItem;
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());

	if (item != m_rootTreeItem)
	{
		CString path;
		while (item != m_rootTreeItem && item != nullptr)
		{
			CString part = tree.GetItemText(item);
			const WCHAR ch = static_cast<const WCHAR>(tree.GetItemData(item));
			path = part + ch + path;
			item = tree.GetParentItem(item);
		}

		doc->m_filterPrefix = CScrypterApp::CStringToUtf8(path);
		doc->UpdateAllViews(this, UPDATEVIEW_RESET);
	}
	
	*pResult = 0;
}
