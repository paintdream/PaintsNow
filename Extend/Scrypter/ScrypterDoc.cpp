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
	// TODO: add one-time construction code here

}

CScrypterDoc::~CScrypterDoc()
{
}

BOOL CScrypterDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	ResetDocument();

	return TRUE;
}

void CScrypterDoc::ResetDocument()
{
	static std::atomic<uint32_t> nextWarp = 0;
	CScrypterApp* app = static_cast<CScrypterApp*>(AfxGetApp());
	Executive& executive = *app->m_executive;
	m_document.Reset(new Document(executive, (nextWarp.fetch_add(1, std::memory_order_relaxed)) % executive.GetKernel().GetWarpCount()));
}

/////////////////////////////////////////////////////////////////////////////
// CScrypterDoc serialization

void CScrypterDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		const String& pluginPath = m_document->GetPluginPath();
		ar.Write(pluginPath.c_str(), pluginPath.size());
	}
	else
	{
		size_t length = (size_t)ar.GetFile()->GetLength();
		String pluginPath;
		pluginPath.resize(length);
		ar.Read(const_cast<char*>(pluginPath.c_str()), (UINT)length);
		ResetDocument();
		m_document->SetPluginPath(pluginPath);
	}
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
