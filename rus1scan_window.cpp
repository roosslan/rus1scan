#include "stdafx.h"

#include "rus1scan_service.h"
#include "rus1scan_window.h"
#include "helper_funcs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/* Crus1scan_window dialog */
Crus1scan_window::Crus1scan_window(CWnd* p_parent /* = nullptr */ ) : CDialog(Crus1scan_window::IDD, p_parent) {
	//{{AFX_DATA_INIT(Crus1scan_window)
	
	//}}AFX_DATA_INIT
}

void Crus1scan_window::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Crus1scan_window)
	DDX_Control(pDX, IDC_COMBO_EVENT, m_combo_box_events);
	DDX_Control(pDX, IDC_COMBO_DEVICE, m_combo_box_devices);
	DDX_Control(pDX, IDC_COMBO_PRINTER, m_combo_box_printers);
	DDX_Control(pDX, IDC_STATIC_DESCRIPTION, m_descr_text);
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_edit_save_directory);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Crus1scan_window, CDialog)
	//{{AFX_MSG_MAP(Crus1scan_window)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &Crus1scan_window::on_btn_browse_click)
	ON_WM_SYSCOMMAND(WM_SYSCOMMAND, CRus1scanWindow::OnSysCommand)
	ON_CBN_SELENDOK(IDC_COMBO_DEVICE, &Crus1scan_window::on_combo_changed)
	ON_CBN_EDITUPDATE(IDC_COMBO_DEVICE, &Crus1scan_window::on_combo_edited) // This one updates immediately
	// ON_COMMAND(IDM_ABOUT, Crus1scan_window::OnAbout)	
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* Crus1scan_window message handlers */
afx_msg void Crus1scan_window::OnSysCommand(const UINT nID, const LPARAM l_param) {

	if ((nID & 0xFFF0) == IDM_ABOUT)
		AfxMessageBox("(c) 2009-2026 www.sborka.dev/rus1scan");
	else {
		CDialog::OnSysCommand(nID, l_param);
	}
}

void Crus1scan_window::on_combo_changed() {
	const int index_selected = m_combo_box_devices.GetCurSel();

	m_combo_box_events.ResetContent();

	for (auto it = rus1WIADev->act_events.begin(); it != rus1WIADev->act_events.end(); ++it) {
		if (index_selected == it->index_of_device)
			m_combo_box_events.AddString(it->btn_event_name.c_str());
	}
}

void Crus1scan_window::on_combo_edited() {
	UpdateData();
}

std::string Crus1scan_window::get_config_file_path() {
	char current_path[MAX_PATH];
	GetModuleFileName(nullptr, current_path, MAX_PATH);

	/* remove filename from the path */
	PathRemoveFileSpec(current_path);
	strcat(current_path, "\\rus1scan.conf");
	return std::string(current_path);
}

BOOL Crus1scan_window::PreTranslateMessage(MSG* pMsg) {
	m_tool_tip_.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL Crus1scan_window::OnInitDialog() {

	CDialog::OnInitDialog();
	HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_MAIN));
	SetIcon(hIcon, FALSE);

	CMenu* hSysMenu = GetSystemMenu(FALSE);
	AppendMenuW(*hSysMenu, MF_STRING, IDM_ABOUT, L"&About...");

	/* Create the ToolTip control */
	if (!m_tool_tip_.Create(this)) {
		TRACE0("Unable to create the ToolTip!");
	}
	else {
		/* Add tool tips to the controls */
		m_tool_tip_.AddTool(&m_edit_save_directory, IDS_SAVEDIR_EDIT);
		m_tool_tip_.Activate(TRUE);
	}

	m_edit_save_directory.SetWindowText("C:\\Scans\\%d.%m.%Y\\");
	// TODO: Edit_SetCueBannerText(m_editDatFormat.GetSafeHwnd(), "DDMMYYYY");

	load_printers(&m_combo_box_printers);			// Loading LPrint
	fill_cb_events(&m_combo_box_devices, &m_combo_box_events, &rus1WIADev->act_events);

	m_combo_box_printers.SetCurSel(0);
	m_combo_box_devices.SetCurSel(0);

	return FALSE;	// return TRUE  unless you set the focus to a control
}

void Crus1scan_window::OnOK() {
	/* Since SHORT is signed, high - order bit equals sign bit.
	 * Therefore, to check if CTRL key is pressed, we check if the value returned by GetKeyState() is negative
	 */
	if (GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0)
		save_action_to_file("true");
	else
		save_action_to_file("false");

	/* Quit the program */
	EndDialog(IDCANCEL);
}

/* "Browse for directory..." button */
void Crus1scan_window::on_btn_browse_click() {

	CFolderPickerDialog m_dlg;
	std::string m_folder;
	if (m_dlg.DoModal() == IDOK) {
		m_folder = m_dlg.GetPathName();
	}
}
