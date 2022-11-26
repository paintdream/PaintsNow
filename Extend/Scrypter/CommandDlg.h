#if !defined(AFX_COMMANDDLG_H__C555A711_4949_4752_A58A_AF8590105C09__INCLUDED_)
#define AFX_COMMANDDLG_H__C555A711_4949_4752_A58A_AF8590105C09__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CommandDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCommandDlg dialog

class CScrypterDoc;
class CCommandDlg : public CDialogBar
{
// Construction
public:
	CCommandDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCommandDlg)
	enum { IDD = IDR_MAINFRAME };
	CProgressCtrl	m_progress;
	CEdit	m_configScript;
	//}}AFX_DATA

	void OnActiveFrameChanged(CWnd* activeWnd);
	void UpdateProgressBar(CScrypterDoc* doc);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommandDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void RefreshDocument();

	// Generated message map functions
	//{{AFX_MSG(CCommandDlg)
	afx_msg LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBrowse();
	afx_msg void OnGo();
	afx_msg void OnStop();
	afx_msg void OnPause();
	afx_msg void OnClear();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMANDDLG_H__C555A711_4949_4752_A58A_AF8590105C09__INCLUDED_)
