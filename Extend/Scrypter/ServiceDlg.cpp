// ServiceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Scrypter.h"
#include "ServiceDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServiceDlg dialog


CServiceDlg::CServiceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServiceDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServiceDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CServiceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServiceDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServiceDlg, CDialog)
	//{{AFX_MSG_MAP(CServiceDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceDlg message handlers
