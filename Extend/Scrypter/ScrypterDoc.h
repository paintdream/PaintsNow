// ScrypterDoc.h : interface of the CScrypterDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRYPTERDOC_H__CAE710BA_53CF_434E_8D85_FD0E9F9F6650__INCLUDED_)
#define AFX_SCRYPTERDOC_H__CAE710BA_53CF_434E_8D85_FD0E9F9F6650__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CScrypterDoc : public CDocument
{
protected: // create from serialization only
	CScrypterDoc();
	DECLARE_DYNCREATE(CScrypterDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScrypterDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	void ResetDocument();

// Implementation
public:
	virtual ~CScrypterDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	PaintsNow::TUnique<PaintsNow::Document>	m_document;
// Generated message map functions
protected:
	//{{AFX_MSG(CScrypterDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRYPTERDOC_H__CAE710BA_53CF_434E_8D85_FD0E9F9F6650__INCLUDED_)
