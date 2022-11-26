// LogView.cpp : implementation file
//

#include "stdafx.h"
#include "Scrypter.h"
#include "LogView.h"
#include "ScrypterDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace PaintsNow;

/////////////////////////////////////////////////////////////////////////////
// CLogView

IMPLEMENT_DYNCREATE(CLogView, CEditView)

CLogView::CLogView()
{
}

CLogView::~CLogView()
{
}


BEGIN_MESSAGE_MAP(CLogView, CEditView)
	//{{AFX_MSG_MAP(CLogView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogView drawing

void CLogView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CLogView diagnostics

#ifdef _DEBUG
void CLogView::AssertValid() const
{
	CEditView::AssertValid();
}

void CLogView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLogView message handlers

void CLogView::OnInitialUpdate() 
{
	CEditView::OnInitialUpdate();
	
	CEdit& edit = GetEditCtrl();
	m_font.CreatePointFont(80, _T("Tahoma"));
	edit.SetFont(&m_font);
	edit.SetReadOnly(TRUE);
}

void CLogView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == UPDATEVIEW_IDLE)
		return;

	CScrypterDoc* doc = static_cast<CScrypterDoc*>(GetDocument());
	CString newLog;
	const std::vector<RuntimeData>& runtimeData = doc->m_document->GetRuntimeData();
	int newLineCount = 0;

	if (m_iterators.size() != runtimeData.size() || lHint == UPDATEVIEW_RESET)
	{
		m_iterators.clear();
		m_iterators.resize(runtimeData.size());
		for (size_t k = 0; k < runtimeData.size(); k++)
		{
			m_iterators[k] = runtimeData[k].logDataItems.begin();
		}

		CEdit& edit = GetEditCtrl();
		edit.SetWindowText(_T(""));
	}

	for (size_t i = 0; i < runtimeData.size(); i++)
	{
		const RuntimeData& data = runtimeData[i];
		TQueueList<LogData>::const_iterator it = m_iterators[i];
		while (it != data.logDataItems.end())
		{
			newLog += CScrypterApp::Utf8ToCString(it->text) + _T("\r\n");
			++it;
			newLineCount++;
		}

		m_iterators[i] = it;
	}

	if (newLineCount != 0)
	{
		const int MAX_LINE = 1024;
		// append text
		CEdit& edit = GetEditCtrl();
		int lineCount = edit.GetLineCount();
		if (lineCount + newLineCount > MAX_LINE)
		{
			// remove old lines
			int n = edit.LineIndex(lineCount + newLineCount - MAX_LINE);
			if (n != -1)
			{
				edit.SetSel(0, n);
				edit.ReplaceSel(_T(""));
			}
		}

		int length = edit.GetWindowTextLength();
		edit.SetSel(length, length);
		edit.ReplaceSel(newLog);
	}
}

