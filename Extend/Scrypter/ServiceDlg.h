#if !defined(AFX_SERVICEDLG_H__55884901_C62E_453F_9E26_E7F9685AFFB1__INCLUDED_)
#define AFX_SERVICEDLG_H__55884901_C62E_453F_9E26_E7F9685AFFB1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServiceDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServiceDlg dialog

class CServiceDlg : public CDialog
{
// Construction
public:
	CServiceDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServiceDlg)
	enum { IDD = IDD_SERVICE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServiceDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVICEDLG_H__55884901_C62E_453F_9E26_E7F9685AFFB1__INCLUDED_)
