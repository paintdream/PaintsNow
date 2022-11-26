// ScrypterView.h : interface of the CScrypterView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRYPTERVIEW_H__5067E9D9_8195_4226_9A5A_05FCA04736AE__INCLUDED_)
#define AFX_SCRYPTERVIEW_H__5067E9D9_8195_4226_9A5A_05FCA04736AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CScrypterView : public CListView
{
protected: // create from serialization only
	CScrypterView();
	DECLARE_DYNCREATE(CScrypterView)

// Attributes
public:
	CScrypterDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScrypterView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CScrypterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	const PaintsNow::RecordData* FindRecord(size_t item);
	bool FilterRecord(const PaintsNow::RecordData& record);

	std::vector<PaintsNow::TQueueList<PaintsNow::RecordData>::const_iterator> m_iterators;
	PaintsNow::TQueueList<const PaintsNow::RecordData*> m_filteredRecordData;
	PaintsNow::TQueueList<const PaintsNow::RecordData*>::const_iterator m_lastFilteredRecordIterator;
	size_t m_lastFilteredRecordIndex;
	size_t m_filteredRecordCount;

// Generated message map functions
protected:
	//{{AFX_MSG(CScrypterView)
	afx_msg void OnOdcachehint(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOdfinditem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ScrypterView.cpp
inline CScrypterDoc* CScrypterView::GetDocument()
   { return (CScrypterDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRYPTERVIEW_H__5067E9D9_8195_4226_9A5A_05FCA04736AE__INCLUDED_)
