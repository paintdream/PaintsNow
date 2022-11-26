// ScrypterDoc.cpp : implementation of the CScrypterDoc class
//

#include "stdafx.h"
#include "Scrypter.h"

#include "ScrypterDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScrypterDoc

IMPLEMENT_DYNCREATE(CScrypterDoc, CDocument)

BEGIN_MESSAGE_MAP(CScrypterDoc, CDocument)
	//{{AFX_MSG_MAP(CScrypterDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

using namespace PaintsNow;

/////////////////////////////////////////////////////////////////////////////
// CScrypterDoc construction/destruction

CScrypterDoc::CScrypterDoc()
{
	m_updateState.store(0, std::memory_order_relaxed);
}

CScrypterDoc::~CScrypterDoc()
{
}

static std::atomic<uint32_t> GNextWarp = 0;
BOOL CScrypterDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	MakeDocument();
	SetModifiedFlag(TRUE);

	return TRUE;
}

void CScrypterDoc::MakeDocument()
{
	if (m_document() == nullptr)
	{
		CScrypterApp* app = static_cast<CScrypterApp*>(AfxGetApp());
		Executive& executive = *app->m_executive;
		Kernel& kernel = executive.GetKernel();
		uint32_t warp = (GNextWarp.fetch_add(1, std::memory_order_relaxed)) % kernel.GetWarpCount();
		m_document.Reset(new Document(executive.GetKernel(), executive.GetArchive(), *executive.GetWorkers()[warp], Wrap(this, &CScrypterDoc::OnDocumentUpdated)));
		m_document->SetWarpIndex(warp);
	}
}

void CScrypterDoc::CompleteDocumentUpdated()
{
	while (m_updateState.exchange(0, std::memory_order_acquire) == 1)
	{
		OnDocumentUpdated(m_document()); // repost to this
	}
}

void CScrypterDoc::OnDocumentUpdated(Document* doc)
{
	ASSERT(m_document() == doc);

	if (m_updateState.exchange(1, std::memory_order_relaxed) == 0)
	{
		m_updateState.store(2, std::memory_order_release);
		AfxGetApp()->GetMainWnd()->PostMessage(WM_UPDATE_ALLVIEWS, 0, reinterpret_cast<LPARAM>(this));
	}
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterDoc serialization
static ZFilterPod GFilterPod;

BOOL CScrypterDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
{
	if (m_document->AcquireLock())
	{
		BOOL ret = CDocument::DoSave(lpszPathName, bReplace);
		m_document->ReleaseLock();
		return ret;
	}
	else
	{
		AfxGetApp()->GetMainWnd()->MessageBox(_T("Failed to save file while document is busy!"));
		return FALSE;
	}
}

void CScrypterDoc::Serialize(CArchive& ar)
{
	MakeDocument();

	bool storing = ar.IsStoring() != 0;

	CFile* file = ar.GetFile();
	m_document->PreSerialize(storing);
	if (storing)
	{
		MemoryStream stream(1024 * 64);
		IStreamBase* filter = GFilterPod.CreateFilter(stream);
		*filter << *m_document;
		file->Write(stream.GetBuffer(), verify_cast<UINT>(stream.GetOffset()));
		filter->Destroy();
	}
	else
	{
		String str;
		str.resize((size_t)file->GetLength());
		StringStream stream(str);
		file->Read(const_cast<char*>(str.data()), verify_cast<UINT>(str.size()));
		IStreamBase* filter = GFilterPod.CreateFilter(stream);
		*filter >> *m_document;
		filter->Destroy();
	}

	m_document->PostSerialize(storing);
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterDoc diagnostics

#ifdef _DEBUG
void CScrypterDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CScrypterDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CScrypterDoc commands

void CScrypterDoc::Clear()
{
	Document* document = m_document();
	if (document->AcquireLock())
	{
		document->Clear();
		document->ReleaseLock();
	}

	UpdateAllViews(nullptr, UPDATEVIEW_RESET);
}

void CScrypterDoc::DeleteContents() 
{
	CDocument::DeleteContents();

	/*
	if (m_document)
	{
		m_document->ClearError();
		if (m_document->AcquireLock())
		{
			m_document->Clear();
			m_document->ReleaseLock();
		}
		else
		{
			AfxGetApp()->GetMainWnd()->MessageBox(_T("Failed to delete contents while document is busy!"));
		}
	}*/
}
