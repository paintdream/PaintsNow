#if !defined(AFX_LOGVIEW_H__FDDBD28E_70A0_493B_BB53_04ABB350E786__INCLUDED_)
#define AFX_LOGVIEW_H__FDDBD28E_70A0_493B_BB53_04ABB350E786__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogView view

class CLogView : public CEditView
{
protected:
	CLogView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CLogView)

// Attributes
public:
	CFont m_font;

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLogView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	std::vector<PaintsNow::TQueueList<PaintsNow::LogData>::const_iterator> m_iterators;

	// Generated message map functions
protected:
	//{{AFX_MSG(CLogView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGVIEW_H__FDDBD28E_70A0_493B_BB53_04ABB350E786__INCLUDED_)
