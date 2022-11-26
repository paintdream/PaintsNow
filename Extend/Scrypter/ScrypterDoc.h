// ScrypterDoc.h : interface of the CScrypterDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRYPTERDOC_H__CAE710BA_53CF_434E_8D85_FD0E9F9F6650__INCLUDED_)
#define AFX_SCRYPTERDOC_H__CAE710BA_53CF_434E_8D85_FD0E9F9F6650__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum
{
	UPDATEVIEW_REFRESH = 0,
	UPDATEVIEW_RESET,
	UPDATEVIEW_REDRAW,
	UPDATEVIEW_IDLE
};

class CScrypterDoc : public CDocument
{
protected: // create from serialization only
	CScrypterDoc();
	DECLARE_DYNCREATE(CScrypterDoc)

// Attributes
public:

// Operations
public:
	void MakeDocument();
	void Clear();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScrypterDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace);
	virtual void DeleteContents();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CScrypterDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	PaintsNow::TShared<PaintsNow::Document>	m_document;
	PaintsNow::String m_filterPrefix;
	std::atomic<size_t> m_updateState;
	void CompleteDocumentUpdated();

protected:
	void OnDocumentUpdated(PaintsNow::Document* document);
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
