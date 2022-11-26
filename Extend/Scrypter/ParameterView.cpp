// ParameterView.cpp : implementation file
//

#include "stdafx.h"
#include "Scrypter.h"
#include "ParameterView.h"
#include "ScrypterDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CParameterView

using namespace PaintsNow;
IMPLEMENT_DYNCREATE(CParameterView, CListView)

CParameterView::CParameterView() : m_editRow(-1), m_editCol(-1), m_parameterVersion(0)
{
}

CParameterView::~CParameterView()
{
}


BEGIN_MESSAGE_MAP(CParameterView, CListView)
	//{{AFX_MSG_MAP(CParameterView)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_WM_KILLFOCUS()
	ON_NOTIFY_REFLECT(NM_RETURN, OnReturn)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
	ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CParameterView drawing

void CParameterView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

void CParameterView::OnInitialUpdate() 
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

	list.DeleteAllItems();
	list.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 60);
	list.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 80);
	list.InsertColumn(2, _T("Value"), LVCFMT_LEFT, 160);
	list.InsertColumn(3, _T("Description"), LVCFMT_LEFT, 140);

	m_edit.Create(WS_CHILD, CRect(0, 0, 5, 5), this, 0);
	m_edit.ShowWindow(SW_HIDE);

	m_parameterVersion = ~(size_t)0;
	CListView::OnInitialUpdate();
}

/////////////////////////////////////////////////////////////////////////////
// CParameterView diagnostics

#ifdef _DEBUG
void CParameterView::AssertValid() const
{
	CListView::AssertValid();
}

void CParameterView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CParameterView message handlers

void CParameterView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == UPDATEVIEW_IDLE)
		return;

	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	Document* document = doc->m_document();
	size_t latestVersion = document->GetParametersVersion();
	if (latestVersion != m_parameterVersion || lHint == UPDATEVIEW_RESET)
	{
		m_parameterVersion = latestVersion;
		if (document->AcquireLock())
		{
			const std::vector<ParameterData>& parameters = document->GetParameters();
			CListCtrl& listCtrl = GetListCtrl();
			listCtrl.DeleteAllItems();

			for (size_t i = 0; i < parameters.size(); i++)
			{
				const ParameterData& parameterData = parameters[i];
				listCtrl.InsertItem(listCtrl.GetItemCount(), CScrypterApp::Utf8ToCString(parameterData.name));
				listCtrl.SetItemText(listCtrl.GetItemCount() - 1, 1, CScrypterApp::Utf8ToCString(parameterData.type));
				listCtrl.SetItemText(listCtrl.GetItemCount() - 1, 2, CScrypterApp::Utf8ToCString(parameterData.value));
				listCtrl.SetItemText(listCtrl.GetItemCount() - 1, 3, CScrypterApp::Utf8ToCString(parameterData.description));
			}
			
			document->ReleaseLock();
		}
	}
}

void CParameterView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	CListCtrl& listCtrl = GetListCtrl();
	CRect rectItem;
	CRect rectList;
	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	Document* document = doc->m_document();

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (pNMListView->iItem != -1)
	{
		// just hardcode here
		if (pNMListView->iSubItem == 2) // clicks 'Value'
		{
			CString type = listCtrl.GetItemText(pNMListView->iItem, 1);
			if (type == "File")
			{
				if (document->AcquireLock())
				{
					CFileDialog file(TRUE, nullptr, nullptr, 0, listCtrl.GetItemText(pNMListView->iItem, 3));
					CScrypterApp* app = static_cast<CScrypterApp*>(AfxGetApp());
					file.m_ofn.lpstrInitialDir = app->m_exeFileFolder;
					if (file.DoModal() == IDOK)
					{
						CString path = app->ConvertToRelativePath(file.GetPathName());
						listCtrl.SetItemText(pNMListView->iItem, pNMListView->iSubItem, path);

						document->GetParameters()[pNMListView->iItem].value = CScrypterApp::CStringToUtf8(path);
					}

					document->ReleaseLock();
					document->InvokeUploadParameters();
				}
			}
			else if (type == "Directory")
			{
				TCHAR targetPath[MAX_PATH] = _T("");
				BROWSEINFO bi;
				bi.hwndOwner = m_hWnd;
				bi.pidlRoot = nullptr;
				bi.pszDisplayName = targetPath;
				bi.lpszTitle = _T("Select folder");
				bi.ulFlags = BIF_EDITBOX;
				bi.lpfn = nullptr;
				bi.lParam = 0;
				bi.iImage = IDR_MAINFRAME;

				if (document->AcquireLock())
				{
					LPITEMIDLIST list = ::SHBrowseForFolder(&bi);
					if (list != nullptr)
					{
						::SHGetPathFromIDList(list, targetPath);
						IMalloc* imalloc = 0;
						if (SUCCEEDED(::SHGetMalloc(&imalloc)))
						{
							imalloc->Free(list);
							imalloc->Release();
						}

						listCtrl.SetItemText(pNMListView->iItem, pNMListView->iSubItem, targetPath);
						CString path = targetPath;
						document->GetParameters()[pNMListView->iItem].value = CScrypterApp::CStringToUtf8(path);
					}

					document->ReleaseLock();
					document->InvokeUploadParameters();
				}
			}
			else
			{
				m_editRow = pNMListView->iItem;
				m_editCol = pNMListView->iSubItem;

				listCtrl.GetSubItemRect(pNMListView->iItem, pNMListView->iSubItem, LVIR_LABEL, rectItem);
				listCtrl.GetWindowRect(&rectList);
				ScreenToClient(&rectList);

				rectItem.left += rectList.left + 3;
				rectItem.top += rectList.top;
				rectItem.right += rectList.left + 3;
				rectItem.bottom += rectList.top + 3;

				CString text = listCtrl.GetItemText(pNMListView->iItem, pNMListView->iSubItem);
				DWORD style = ::GetWindowLong(m_edit.m_hWnd, GWL_STYLE);
				if (type == "Number")
				{
					style |= ES_NUMBER;
				}
				else
				{
					style &= ~ES_NUMBER;
				}

				::SetWindowLong(m_edit.m_hWnd, GWL_STYLE, style);

				m_edit.SetWindowText(text);
				m_edit.ShowWindow(SW_SHOW);
				m_edit.MoveWindow(&rectItem);
				m_edit.SetFocus();
				m_edit.CreateSolidCaret(1, rectItem.Height() - 5);
				m_edit.ShowCaret();
				m_edit.SetSel(-1);
			}
		}
	}
	else
	{
		m_editRow = m_editCol = -1;
	}

	*pResult = 0;
}

void CParameterView::ConfirmEditInput()
{
	if (m_editRow != -1)
	{
		CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
		Document* document = doc->m_document();
		CListCtrl& listCtrl = GetListCtrl();
		CString text = "";
		m_edit.GetWindowText(text);
		m_edit.ShowWindow(SW_HIDE);

		if (document->AcquireLock())
		{
			listCtrl.SetItemText(m_editRow, m_editCol, text);
			document->GetParameters()[m_editRow].value = CScrypterApp::CStringToUtf8(text);
			document->ReleaseLock();
			document->InvokeUploadParameters();
		}

		m_editRow = m_editCol = -1;
	}
}

void CParameterView::OnKillFocus(CWnd* pNewWnd) 
{
	CListView::OnKillFocus(pNewWnd);

	if (pNewWnd != &m_edit)
	{
		ConfirmEditInput();
	}
}

void CParameterView::OnReturn(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// ConfirmEditInput();	
	*pResult = 0;
}

void CParameterView::OnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// ConfirmEditInput();	
	*pResult = 0;
}

void CParameterView::OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ConfirmEditInput();	
	*pResult = 0;
}
