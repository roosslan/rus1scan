// rus1scan_service.cpp
#include "stdafx.h"

#include "rus1scan_service.h"
#include "rus1scan_window.h"
#include "rus1scan.h"
#include "helper_funcs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SERVICE_STATUS_HANDLE status_handle  = nullptr;
SERVICE_STATUS        service_status = { 0 };
HANDLE                service_stop_event = INVALID_HANDLE_VALUE;

VOID WINAPI  ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI  ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lp_param);

#define SERVICE_NAME  _T("rus1scan")

// Crus1scan_service
BEGIN_MESSAGE_MAP(Crus1scan_service, CWinApp)
	//{{AFX_MSG_MAP(Crus1scan_service)
	//}}AFX_MSG
	// ON_COMMAND(ID_HELP, CWinApp::OnHelp)	
END_MESSAGE_MAP()

Crus1scan_service::Crus1scan_service() { }


/* The one and only Crus1scan_service object */
Crus1scan_service rus1scan_service;

static auto launch_debugger() -> bool {
	std::wstring system_dir(MAX_PATH + 1, '\0');
	const UINT n_chars = GetSystemDirectoryW(system_dir.data(), system_dir.length());
	if (n_chars == 0)
		return false;
	system_dir.resize(n_chars);

	// Get process ID and create the command line
	DWORD pid = GetCurrentProcessId();
	std::wostringstream s;
	s << system_dir << L"\\vsjitdebugger.exe -p " << pid;
	std::wstring cmd_line = s.str();

	/* Start debugger process */
	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcessW(nullptr, &cmd_line[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
		return false;

	/* Close debugger process handles to eliminate resource leak */
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	// Wait for the debugger to attach
	while (!IsDebuggerPresent()) Sleep(100);

	// Stop execution so the debugger can take over
	DebugBreak();
	return true;
}

/* Crus1scan_service initialization */
BOOL Crus1scan_service::InitInstance() {
	// Standard initialization
#ifdef _AFXDLL
	// CWinApp::Enable3dControls is no longer needed.You should remove this call
	// Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	ru_logger::init_logging();
	LOG_SAVE << "InitInstance: Entry";

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	/*if (SUCCEEDED(hr))
	* {
	*	// Create the main window and enter the message loop
	*	RunGetImage();
	*	// Close the COM library 
	*	CoUninitialize();
	* }
	*/
	IWiaDevMgr* ru_wia_mgr;
	ru_wia_dev = new Crus1scan(&ru_wia_mgr);

	constexpr SERVICE_TABLE_ENTRY service_table[] =
	{
		{_T("rus1scan"), static_cast<LPSERVICE_MAIN_FUNCTIONA>(ServiceMain)},
		{nullptr, nullptr}
	};

	if (StartServiceCtrlDispatcher(service_table) == true) {
		LOG_SAVE << "InitInstance: StartServiceCtrlDispatcher: Launched as service";

		m_pEventCallback = new WiaWrap::CEventCallback;

		if (m_pEventCallback != nullptr) {
			LOG_SAVE << "InitInstance: Register pEventCallback";
			m_pEventCallback->Register();
		}

		LOG_SAVE << GetLastError();
		// return FALSE;
	}
	else {
		/* «апущено как приложение */

		Crus1scan_window rus1scan_window;
		m_pMainWnd = &rus1scan_window;
		rus1scan_window.rus1WIADev = ru_wia_dev;

		if (const int nResponse = rus1scan_window.DoModal(); nResponse == IDOK) {}
		else if (nResponse == IDCANCEL)	{
		}
	}

	/* close the COM library */
	CoUninitialize();

	LOG_SAVE << "InitInstance: Exit";

	/* Since the dialog has been closed, return FALSE so that we exit the
	 * application, rather than start the application's message pump.
	 */
	return false;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
	LOG_SAVE << "ServiceMain: Entry";

	// CComPtr<WiaWrap::CEventCallback> srv_pEventCallback = new WiaWrap::CEventCallback;

	// HRESULT hr = CoInitialize(NULL);

	// if (srv_pEventCallback != NULL)
	// {
	//	 LOG_SAVE << "ServiceMain: Register pEventCallback";
	//	 srv_pEventCallback->Register();
	// }
	// else LOG_SAVE << "ServiceMain: srv_pEventCallback is NULL";
    //
	// IWiaDevMgr* RooWiaMgr;	
	// Crus1scan* RooWiaDev = new Crus1scan(NULL, &RooWiaMgr);

	status_handle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (status_handle == nullptr) {
		LOG_SAVE << "ServiceMain: RegisterServiceCtrlHandler returned error";
		return;
	}

	/* Tell the service controller we are starting */
	ZeroMemory(&service_status, sizeof(service_status));
	service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	service_status.dwControlsAccepted = 0;
	service_status.dwCurrentState = SERVICE_START_PENDING;
	service_status.dwWin32ExitCode = 0;
	service_status.dwServiceSpecificExitCode = 0;
	service_status.dwCheckPoint = 0;

	if (SetServiceStatus(status_handle, &service_status) == false) {
		LOG_SAVE << "ServiceMain: SetServiceStatus returned error";
	}

	LOG_SAVE << "ServiceMain: Performing Service Start Operations";
	IID r_guid;
	LOG_SAVE << "ServiceMain:IIDFromString";
	// const CLSID = IID("{73856D9A - 2720 - 487A - A584 - 21D5774E9D0F}");
	IIDFromString(L"0000001B00000000C000000000000046", &r_guid);
	LOG_SAVE << "ServiceMain: QueryInterface";
	// HRESULT hhR = srv_pEventCallback->QueryInterface(rGuid, NULL);
	// LOG_SAVE << hhR;
	// ::CoTaskMemFree(&rGuid);


	// HRESULT hr = CoInitialize(NULL);
	// if (m_pEventCallback != NULL)
	// {
	//     LOG_SAVE << "ServiceMain: Register pEventCallback";
	//     m_pEventCallback->Register();
	// }

	/* Create stop event to wait on later */
	service_stop_event = CreateEvent(nullptr, true, false, nullptr);
	if (service_stop_event == nullptr)
	{
		LOG_SAVE << "ServiceMain: CreateEvent(service_stop_event) returned error";

		service_status.dwControlsAccepted = 0;
		service_status.dwCurrentState = SERVICE_STOPPED;
		service_status.dwWin32ExitCode = GetLastError();
		service_status.dwCheckPoint = 1;

		if (SetServiceStatus(status_handle, &service_status) == false)
			LOG_SAVE << "ServiceMain: SetServiceStatus returned error";

		return;
	}

	/* Tell the service controller we are started */
	service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	service_status.dwCurrentState = SERVICE_RUNNING;
	service_status.dwWin32ExitCode = 0;
	service_status.dwCheckPoint = 0;

	if (SetServiceStatus(status_handle, &service_status) == false) {
		LOG_SAVE << "ServiceMain: SetServiceStatus returned error";
	}

	/* Start the thread that will perform the main task of the service */
	const HANDLE hThread = CreateThread(nullptr, 0, ServiceWorkerThread, nullptr, 0, nullptr);

	LOG_SAVE << "ServiceMain: Waiting for Worker Thread to complete";

	/* Wait until our worker thread exits effectively signaling that the service needs to stop */
	WaitForSingleObject(hThread, INFINITE);
	LOG_SAVE << "ServiceMain: Worker Thread Stop Event signaled";

	LOG_SAVE << "ServiceMain: Performing Cleanup Operations";
	CloseHandle(service_stop_event);

	service_status.dwControlsAccepted = 0;
	service_status.dwCurrentState = SERVICE_STOPPED;
	service_status.dwWin32ExitCode = 0;
	service_status.dwCheckPoint = 3;

	if (SetServiceStatus(status_handle, &service_status) == false) {
		LOG_SAVE << "ServiceMain: SetServiceStatus returned error";
	}

	LOG_SAVE << "ServiceMain: Exit";
}


void WINAPI ServiceCtrlHandler(const DWORD ctrl_code) {
	LOG_SAVE << "ServiceCtrlHandler: Entry";

	if (ctrl_code == SERVICE_CONTROL_STOP && service_status.dwCurrentState != SERVICE_RUNNING) {

		LOG_SAVE << "ServiceCtrlHandler: SERVICE_CONTROL_STOP Request";

		/* perform tasks necessary to stop the service here */
		service_status.dwControlsAccepted = 0;
		service_status.dwCurrentState = SERVICE_STOP_PENDING;
		service_status.dwWin32ExitCode = 0;
		service_status.dwCheckPoint = 4;

		if (SetServiceStatus(status_handle, &service_status) == false)
			LOG_SAVE << "ServiceCtrlHandler: SetServiceStatus returned error";

		/* This will signal the worker thread to start shutting down */
		SetEvent(service_stop_event);
	}

	LOG_SAVE << "ServiceCtrlHandler: Exit";
}


DWORD WINAPI ServiceWorkerThread(LPVOID lp_param) {
	launch_debugger();
	LOG_SAVE << "ServiceWorkerThread: Entry";

	// HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	// CComPtr<WiaWrap::CEventCallback>  srv_pEventCallback;
	// srv_pEventCallback = new WiaWrap::CEventCallback;

	// if (srv_pEventCallback != NULL)
	// {
	//	 LOG_SAVE << "ServiceWorkerThread: Register pEventCallback";
	//	 srv_pEventCallback->Register();
	// }
	// else LOG_SAVE << "ServiceWorkerThread: srv_pEventCallback is NULL";
	// LOG_SAVE << GetLastError();


	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(service_stop_event, 0) != WAIT_OBJECT_0)
	{
		//LOG_SAVE << "ServiceWorkerThread: WaitForSingleObject";
		Sleep(3000); // ... каждые 3 секунды
	}

	LOG_SAVE << "ServiceWorkerThread: Exit";

	/* close the COM library */
	CoUninitialize();

	return ERROR_SUCCESS;
}