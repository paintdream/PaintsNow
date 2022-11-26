// Scrypter.h : main header file for the SCRYPTER application
//

#if !defined(AFX_SCRYPTER_H__A710491C_1C5B_4111_A946_ED37DC7BC304__INCLUDED_)
#define AFX_SCRYPTER_H__A710491C_1C5B_4111_A946_ED37DC7BC304__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CScrypterApp:
// See Scrypter.cpp for the implementation of this class
//

class CScrypterApp : public CWinApp
{
public:
	CScrypterApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScrypterApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

	CString ConvertToRelativePath(const CString& inputPath);
	CString m_exeFilePath;
	CString m_exeFileFolder;
	PaintsNow::TUnique<PaintsNow::Executive> m_executive;

	static CString Utf8ToCString(PaintsNow::StringView str);
	static PaintsNow::String CStringToUtf8(const CString& str);

// Implementation
	//{{AFX_MSG(CScrypterApp)
	afx_msg void OnAppAbout();
	afx_msg void OnConfigService();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRYPTER_H__A710491C_1C5B_4111_A946_ED37DC7BC304__INCLUDED_)
