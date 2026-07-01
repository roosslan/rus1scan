// rus1scan_service.h

#if !defined(AFX_RUS1SCAN_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_)
#define AFX_RUS1SCAN_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "stdafx.h"
#include "rus1scan.h"
#include "EventCallback.h"

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

/////////////////////////////////////////////////////////////////////////////
// Crus1scan_service:
// See rus1scan_service.cpp for the implementation of this class

class Crus1scan_service : public CWinApp {
	CComPtr<WiaWrap::CEventCallback>  m_pEventCallback;
public:
	Crus1scan_service();
	Crus1scan* ru_wia_dev;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Crus1scan_service)	
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL
	// Implementation
	//{{AFX_MSG(Crus1scan_service)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()	
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RUS1SCAN_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_)
