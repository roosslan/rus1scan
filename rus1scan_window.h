// rus1scan_window.h: header file

#if !defined(AFX_RUS1SCANWINDOW_H__115F4226_5CD5_11D1_00A0243D1382__INCLUDED_)
#define AFX_RUS1SCANWINDOW_H__115F4226_5CD5_11D1_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "stdafx.h"
#include "resource.h"
#include "rus1scan.h"
#include "helper_funcs.h"

// Imaging Device
// Class = Image
// ClassGuid = { 6bdd1fc6 - 810f - 11d0 - bec7 - 08002be2092f }
// This class includes still - image capture devices, digital cameras, and scanners.


/* Crus1scan_window (dialog window) */
class Crus1scan_window final : public CDialog {
	BOOL  m_bDisplayWaitCursor;
	afx_msg void OnSysCommand(UINT nID, LPARAM l_param);
	//	void SaveCmdToFile(bool asAdministrator);
	//	void LoadCmdFromFile();
	std::string get_config_file_path();
public:	
	Crus1scan* rus1WIADev;
	explicit Crus1scan_window(CWnd* p_parent = NULL);	// standard constructor
// Dialog Data
	//{{AFX_DATA(Crus1scan_window)
	enum { IDD = IDD_RUS1SCAN_DIALOG };
	CComboBox m_combo_box_events;
	CComboBox m_combo_box_devices;
	CComboBox m_combo_box_printers;
	CStatic m_descr_text;
	//}}AFX_DATA
	CEdit m_edit_save_directory;
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Crus1scan_window)
protected:
	CToolTipCtrl m_tool_tip_;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL	
	// Generated message map functions
	//{{AFX_MSG(Crus1scan_window)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void on_combo_changed();
	void on_combo_edited();
	//}}AFX_MSG
	afx_msg void on_btn_browse_click();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RUS1SCANWINDOW_H__115F4226_5CD5_11D1_00A0243D1382__INCLUDED_)
