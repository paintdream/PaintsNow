#if !defined(AFX_MONITORVIEW_H__4E67C413_0B6C_43E2_A52A_951C8825D0A4__INCLUDED_)
#define AFX_MONITORVIEW_H__4E67C413_0B6C_43E2_A52A_951C8825D0A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CMonitorView : public CListView
{
protected: // create from serialization only
	CMonitorView();
	DECLARE_DYNCREATE(CMonitorView)

// Attributes
public:
	CScrypterDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMonitorView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMonitorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	std::vector<PaintsNow::TQueueList<PaintsNow::LogData>::const_iterator> m_iterators;

// Generated message map functions
protected:
	//{{AFX_MSG(CMonitorView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in MonitorView.cpp
inline CScrypterDoc* CMonitorView::GetDocument()
   { return (CScrypterDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MONITORVIEW_H__4E67C413_0B6C_43E2_A52A_951C8825D0A4__INCLUDED_)
