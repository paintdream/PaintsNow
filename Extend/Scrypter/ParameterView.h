#if !defined(AFX_PARAMETERVIEW_H__24FAFA2E_0BF6_4323_BAEF_1B7714529A96__INCLUDED_)
#define AFX_PARAMETERVIEW_H__24FAFA2E_0BF6_4323_BAEF_1B7714529A96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ParameterView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CParameterView view

class CParameterView : public CListView
{
protected:
	CParameterView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CParameterView)

// Attributes
public:
	CEdit m_edit;
	int m_editRow;
	int m_editCol;
	size_t m_parameterVersion;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParameterView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	void ConfirmEditInput();

// Implementation
protected:
	virtual ~CParameterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CParameterView)
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnReturn(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARAMETERVIEW_H__24FAFA2E_0BF6_4323_BAEF_1B7714529A96__INCLUDED_)
