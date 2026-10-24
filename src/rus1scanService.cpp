// rus1scanService.cpp: приложение rus1scan - выбор режима работы, служба Windows,
// установка/удаление службы (см. rus1scanService.h).

#include "stdafx.h"

#include "rus1scanService.h"
#include "rus1scanWindow.h"
#include "service_core.h"
#include "actions.h"
#include "config.h"
#include "strutil.h"
#include "version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Современный вид стандартных элементов управления (Common Controls 6)
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace
{
    const wchar_t* const kServiceName = L"rus1scan";
    const wchar_t* const kServiceDisplayName = L"rus1scan - scanner buttons";
    const wchar_t* const kServiceDescription =
        L"Handles scanner button events (WIA): scans and saves, prints or opens the image. "
        L"Settings: rus1scan.toml next to rus1scan.exe (double-click rus1scan.exe to edit).";

    SERVICE_STATUS_HANDLE g_statusHandle = NULL;
    SERVICE_STATUS        g_status = {};
    std::mutex            g_statusMutex;
    HANDLE                g_stopEvent = NULL;     // Ручной сброс; взводится при остановке
    HANDLE                g_consoleDone = NULL;   // Режим /console: рабочий поток завершён
    DWORD                 g_serviceExitCode = 0;

    // Сообщает SCM текущее состояние службы.
    //   state    - SERVICE_START_PENDING / SERVICE_RUNNING / SERVICE_STOP_PENDING / SERVICE_STOPPED;
    //   exitCode - код завершения для SERVICE_STOPPED (0 - успех; иначе передаётся как
    //              ERROR_SERVICE_SPECIFIC_ERROR + dwServiceSpecificExitCode);
    //   waitHint - сколько миллисекунд SCM должен ждать следующего сообщения в состоянии *_PENDING.
    void ReportServiceStatus(DWORD state, DWORD exitCode, DWORD waitHint)
    {
        std::lock_guard<std::mutex> lock(g_statusMutex);
        if (g_statusHandle == NULL)
        {
            return;
        }

        g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        g_status.dwCurrentState = state;
        g_status.dwControlsAccepted = (state == SERVICE_RUNNING) ? (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN) : 0;
        g_status.dwWin32ExitCode = (exitCode == 0) ? NO_ERROR : ERROR_SERVICE_SPECIFIC_ERROR;
        g_status.dwServiceSpecificExitCode = exitCode;
        g_status.dwWaitHint = waitHint;
        g_status.dwCheckPoint = (state == SERVICE_RUNNING || state == SERVICE_STOPPED) ? 0 : g_status.dwCheckPoint + 1;

        if (!SetServiceStatus(g_statusHandle, &g_status))
        {
            LOG_SAVE << "SetServiceStatus(" << state << ") failed, error " << GetLastError();
        }
    }

    // Обработчик команд SCM (остановка, завершение работы Windows, опрос состояния).
    // Вызывается в главном потоке процесса (внутри StartServiceCtrlDispatcher).
    //   control   - код команды (SERVICE_CONTROL_...);
    //   eventType - дополнительный код (не используется);
    //   eventData - данные команды (не используются);
    //   context   - контекст из RegisterServiceCtrlHandlerEx (не используется).
    // Возвращает NO_ERROR или ERROR_CALL_NOT_IMPLEMENTED для неподдерживаемых команд.
    DWORD WINAPI ServiceCtrlHandlerEx(DWORD control, DWORD eventType, LPVOID eventData, LPVOID context)
    {
        UNREFERENCED_PARAMETER(eventType);
        UNREFERENCED_PARAMETER(eventData);
        UNREFERENCED_PARAMETER(context);

        switch (control)
        {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            LOG_SAVE << (control == SERVICE_CONTROL_STOP ? "SERVICE_CONTROL_STOP" : "SERVICE_CONTROL_SHUTDOWN");
            ReportServiceStatus(SERVICE_STOP_PENDING, 0, 30000);
            SetEvent(g_stopEvent);
            return NO_ERROR;

        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
        }
    }

#ifdef _DEBUG
    // Только для Debug-сборки: запускает vsjitdebugger и ждёт подключения отладчика
    // не дольше timeoutMs. Включается параметром запуска службы: sc start rus1scan /debug
    // (в сеансе 0 окно vsjitdebugger может быть не видно).
    //   timeoutMs - сколько ждать подключения отладчика.
    // Возвращает true, если отладчик подключился.
    bool LaunchDebugger(DWORD timeoutMs)
    {
        wchar_t systemDir[MAX_PATH] = {};
        if (GetSystemDirectoryW(systemDir, MAX_PATH) == 0)
        {
            return false;
        }

        std::wstring command = std::wstring(L"\"") + systemDir + L"\\vsjitdebugger.exe\" -p " +
                               std::to_wstring(GetCurrentProcessId());
        std::vector<wchar_t> buffer(command.begin(), command.end());
        buffer.push_back(L'\0');

        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        if (!CreateProcessW(NULL, buffer.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            LOG_SAVE << "LaunchDebugger: CreateProcess failed, error " << GetLastError();
            return false;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        const ULONGLONG deadline = GetTickCount64() + timeoutMs;
        while (!IsDebuggerPresent())
        {
            if (GetTickCount64() >= deadline)
            {
                LOG_SAVE << "LaunchDebugger: debugger did not attach within " << timeoutMs << " ms, continuing";
                return false;
            }
            Sleep(100);
        }
        DebugBreak();
        return true;
    }
#endif

    // Точка входа службы, вызывается SCM в отдельном потоке.
    //   argc - число параметров запуска службы (включая имя службы);
    //   argv - параметры запуска (sc start rus1scan <параметры>); "/debug" в Debug-сборке
    //          запускает отладчик.
    VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv)
    {
        g_statusHandle = RegisterServiceCtrlHandlerExW(kServiceName, ServiceCtrlHandlerEx, NULL);
        if (g_statusHandle == NULL)
        {
            LOG_SAVE << "RegisterServiceCtrlHandlerEx failed, error " << GetLastError();
            return;
        }

        g_stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (g_stopEvent == NULL)
        {
            const DWORD error = GetLastError();
            LOG_SAVE << "CreateEvent failed, error " << error;
            ReportServiceStatus(SERVICE_STOPPED, error, 0);
            return;
        }

        ReportServiceStatus(SERVICE_START_PENDING, 0, 5000);
        LOG_SAVE << "Service starting, version " << RUS1SCAN_VERSION_STRING << ", exe: " << strutil::GetModulePath();

#ifdef _DEBUG
        for (DWORD i = 1; i < argc; ++i)
        {
            if (argv[i] != NULL && _wcsicmp(argv[i], L"/debug") == 0)
            {
                ReportServiceStatus(SERVICE_START_PENDING, 0, 35000);
                LaunchDebugger(30000);
            }
        }
#else
        UNREFERENCED_PARAMETER(argc);
        UNREFERENCED_PARAMETER(argv);
#endif

        DWORD exitCode = 0;
        {
            CServiceCore core(g_stopEvent);
            if (!core.Start())
            {
                ReportServiceStatus(SERVICE_STOPPED, core.ExitCode(), 0);
                return;
            }
            ReportServiceStatus(SERVICE_RUNNING, 0, 0);
            LOG_SAVE << "Service running";

            core.Wait();
            exitCode = core.ExitCode();
        }

        g_serviceExitCode = exitCode;
        LOG_SAVE << "Service stopped, exit code " << exitCode;
        ReportServiceStatus(SERVICE_STOPPED, exitCode, 0);
    }

    // Обработчик Ctrl+C / Ctrl+Break / закрытия окна консоли в режиме /console.
    // Взводит событие остановки и даёт рабочему потоку до 5 секунд на завершение.
    //   ctrlType - тип сигнала консоли.
    // Возвращает TRUE (сигнал обработан).
    BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType)
    {
        LOG_SAVE << "Console control signal " << ctrlType << " - stopping";
        SetEvent(g_stopEvent);
        if (g_consoleDone != NULL)
        {
            WaitForSingleObject(g_consoleDone, 5000);
        }
        return TRUE;
    }

    // Поток режима /console: читает стандартный ввод и останавливает работу по Enter
    // или при закрытии ввода (удобно для запуска из скриптов).
    //   param - дескриптор стандартного ввода.
    DWORD WINAPI ConsoleInputThread(LPVOID param)
    {
        HANDLE input = static_cast<HANDLE>(param);
        char buffer[256];
        DWORD read = 0;
        for (;;)
        {
            if (!ReadFile(input, buffer, sizeof(buffer), &read, NULL) || read == 0)
            {
                break;      // Ввод закрыт
            }
            if (memchr(buffer, '\n', read) != nullptr || memchr(buffer, '\r', read) != nullptr)
            {
                break;      // Нажат Enter
            }
        }
        LOG_SAVE << "Console input finished - stopping";
        SetEvent(g_stopEvent);
        return 0;
    }

    // Возвращает true, если стандартный вывод процесса куда-то направлен
    // (консоль, канал или файл). У GUI-приложения это так, только если родитель
    // передал дескрипторы (перенаправление в cmd/PowerShell/bash).
    bool HasStdOutput()
    {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        return out != NULL && out != INVALID_HANDLE_VALUE && GetFileType(out) != FILE_TYPE_UNKNOWN;
    }

    // Выводит строку в стандартный вывод (UTF-8 в канал/файл, UTF-16 в консоль).
    //   text - выводимый текст (перевод строки добавляется).
    void WriteStdOut(const std::wstring& text)
    {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        const std::wstring line = text + L"\r\n";
        DWORD written = 0;
        if (!WriteConsoleW(out, line.c_str(), static_cast<DWORD>(line.size()), &written, NULL))
        {
            const std::string utf8 = strutil::WideToUtf8(line);
            WriteFile(out, utf8.data(), static_cast<DWORD>(utf8.size()), &written, NULL);
        }
    }
}

BEGIN_MESSAGE_MAP(CRus1scanService, CWinApp)
END_MESSAGE_MAP()

// Единственный объект приложения
CRus1scanService theApp;

// Конструктор приложения. Вся инициализация выполняется в InitInstance.
CRus1scanService::CRus1scanService()
{
}

// Точка входа MFC: разбирает командную строку и запускает нужный режим (см. rus1scanService.h).
// Всегда возвращает FALSE - приложение завершается после выхода из режима, код
// завершения процесса возвращается из ExitInstance.
BOOL CRus1scanService::InitInstance()
{
    CWinApp::InitInstance();

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args;
    for (int i = 1; argv != NULL && i < argc; ++i)
    {
        args.push_back(argv[i]);
    }
    if (argv != NULL)
    {
        LocalFree(argv);
    }

    std::wstring stiDevice;
    std::wstring stiEvent;
    std::vector<std::wstring> rest;
    for (const auto& arg : args)
    {
        if (_wcsicmp(arg.c_str(), L"/quiet") == 0)
        {
            m_quiet = true;
        }
        else if (_wcsnicmp(arg.c_str(), L"/StiDevice:", 11) == 0)
        {
            stiDevice = arg.substr(11);
        }
        else if (_wcsnicmp(arg.c_str(), L"/StiEvent:", 10) == 0)
        {
            stiEvent = arg.substr(10);
        }
        else
        {
            rest.push_back(arg);
        }
    }

    if (!stiDevice.empty() || !stiEvent.empty())
    {
        m_exitCode = RunStiLaunch(stiDevice, stiEvent);
    }
    else if (rest.empty())
    {
        m_exitCode = RunServiceOrGui();
    }
    else if (_wcsicmp(rest[0].c_str(), L"/install") == 0)
    {
        m_exitCode = RunInstall();
    }
    else if (_wcsicmp(rest[0].c_str(), L"/uninstall") == 0)
    {
        m_exitCode = RunUninstall();
    }
    else if (_wcsicmp(rest[0].c_str(), L"/console") == 0)
    {
        m_exitCode = RunConsole();
    }
    else if (_wcsicmp(rest[0].c_str(), L"/process") == 0)
    {
        m_exitCode = RunProcessFile(rest);
    }
    else
    {
        rLogger::InitLogging(L"app");
        ShowMessage(L"Неизвестный параметр: " + rest[0] + L"\n\n"
                    L"rus1scan.exe                 - окно настроек (или служба, если запущен SCM)\n"
                    L"rus1scan.exe /install        - установить и запустить службу\n"
                    L"rus1scan.exe /uninstall      - остановить и удалить службу\n"
                    L"rus1scan.exe /console        - режим службы в консоли (Enter или Ctrl+C - выход)\n"
                    L"rus1scan.exe /process <файл.bmp> <GUID события> [ID устройства]\n"
                    L"                             - выполнить действие привязки над BMP-файлом",
                    true);
        m_exitCode = 2;
    }

    return FALSE;
}

// Вызывается MFC при завершении приложения. Возвращает код завершения процесса,
// выставленный выбранным режимом.
int CRus1scanService::ExitInstance()
{
    CWinApp::ExitInstance();
    return m_exitCode;
}

// Пытается подключиться к SCM как служба. Если exe запущен не SCM (двойной клик),
// StartServiceCtrlDispatcher сразу возвращает ошибку - тогда открывается окно настроек.
// Возвращает код завершения.
int CRus1scanService::RunServiceOrGui()
{
    rLogger::InitLogging(L"svc");

    SERVICE_TABLE_ENTRYW table[] =
    {
        { const_cast<LPWSTR>(kServiceName), ServiceMain },
        { NULL, NULL }
    };

    if (StartServiceCtrlDispatcherW(table))
    {
        return static_cast<int>(g_serviceExitCode);    // Служба отработала и остановлена
    }

    const DWORD error = GetLastError();
    rLogger::InitLogging(L"gui");
    if (error != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
    {
        LOG_SAVE << "StartServiceCtrlDispatcher failed, error " << error << " - opening settings window";
    }
    return RunGui();
}

// Режим окна настроек: COM в однопоточном апартаменте (нужен диалогам оболочки)
// и модальное окно CRus1scanWindow. Возвращает 0.
int CRus1scanService::RunGui()
{
    LOG_SAVE << "Settings window started";
    const HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    {
        CRus1scanWindow window;
        m_pMainWnd = &window;
        window.DoModal();
        m_pMainWnd = NULL;
    }

    if (SUCCEEDED(hr))
    {
        CoUninitialize();
    }
    LOG_SAVE << "Settings window closed";
    return 0;
}

// Режим /console: то же, что служба, но в консольном окне с выводом журнала.
// Остановка - Ctrl+C, закрытие окна, Enter или закрытие стандартного ввода.
// Возвращает код завершения рабочего потока.
int CRus1scanService::RunConsole()
{
    if (!HasStdOutput())
    {
        AllocConsole();
    }
    rLogger::InitLogging(L"console", true);
    LOG_SAVE << "Console mode started (Enter or Ctrl+C to stop)";

    g_stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_consoleDone = CreateEventW(NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (input != NULL && input != INVALID_HANDLE_VALUE && GetFileType(input) != FILE_TYPE_UNKNOWN)
    {
        HANDLE thread = ::CreateThread(NULL, 0, ConsoleInputThread, input, 0, NULL);
        if (thread != NULL)
        {
            CloseHandle(thread);
        }
    }

    DWORD exitCode = 0;
    {
        CServiceCore core(g_stopEvent);
        if (core.Start())
        {
            core.Wait();
        }
        exitCode = core.ExitCode();
    }

    SetEvent(g_consoleDone);
    LOG_SAVE << "Console mode finished, exit code " << exitCode;
    return static_cast<int>(exitCode);
}

// Режим /install: регистрирует службу "rus1scan" (автозапуск, зависит от службы WIA
// stisvc, перезапуск при сбое) и запускает её. Если служба уже есть - обновляет путь
// к exe и запускает. Требует прав администратора.
// Возвращает 0 при успехе, иначе код ошибки Win32.
int CRus1scanService::RunInstall()
{
    rLogger::InitLogging(L"setup");

    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (scm == NULL)
    {
        const DWORD error = GetLastError();
        ShowMessage(L"Не удалось открыть диспетчер служб (ошибка " + std::to_wstring(error) +
                    L"). Запустите rus1scan.exe /install от имени администратора.", true);
        return static_cast<int>(error);
    }

    const std::wstring binaryPath = L"\"" + strutil::GetModulePath() + L"\"";
    SC_HANDLE service = CreateServiceW(scm, kServiceName, kServiceDisplayName,
                                       SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG | SERVICE_START,
                                       SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                       binaryPath.c_str(), NULL, NULL, L"stisvc\0", NULL, NULL);
    bool existed = false;
    if (service == NULL && GetLastError() == ERROR_SERVICE_EXISTS)
    {
        existed = true;
        service = OpenServiceW(scm, kServiceName, SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG | SERVICE_START);
        if (service != NULL)
        {
            ChangeServiceConfigW(service, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE,
                                 binaryPath.c_str(), NULL, NULL, L"stisvc\0", NULL, NULL, kServiceDisplayName);
        }
    }
    if (service == NULL)
    {
        const DWORD error = GetLastError();
        CloseServiceHandle(scm);
        ShowMessage(L"Не удалось создать службу (ошибка " + std::to_wstring(error) + L").", true);
        return static_cast<int>(error);
    }

    SERVICE_DESCRIPTIONW description = { const_cast<LPWSTR>(kServiceDescription) };
    ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &description);

    SC_ACTION restart[3] = { { SC_ACTION_RESTART, 60000 }, { SC_ACTION_RESTART, 60000 }, { SC_ACTION_NONE, 0 } };
    SERVICE_FAILURE_ACTIONSW failure = {};
    failure.dwResetPeriod = 24 * 60 * 60;
    failure.cActions = 3;
    failure.lpsaActions = restart;
    ChangeServiceConfig2W(service, SERVICE_CONFIG_FAILURE_ACTIONS, &failure);

    DWORD startError = NO_ERROR;
    if (!StartServiceW(service, 0, NULL))
    {
        startError = GetLastError();
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    LOG_SAVE << "Service " << (existed ? "updated" : "installed") << ": " << binaryPath << ", start: " << startError;
    if (startError != NO_ERROR && startError != ERROR_SERVICE_ALREADY_RUNNING)
    {
        ShowMessage(L"Служба rus1scan установлена, но не запустилась (ошибка " + std::to_wstring(startError) +
                    L"). Подробности - в журнале " + rLogger::GetLogFolderPath(), true);
        return static_cast<int>(startError);
    }
    ShowMessage(std::wstring(L"Служба rus1scan ") + (existed ? L"обновлена" : L"установлена") + L" и запущена.", false);
    return 0;
}

// Режим /uninstall: останавливает службу (ждёт до 30 секунд) и удаляет её.
// Требует прав администратора. Возвращает 0 при успехе, иначе код ошибки Win32.
int CRus1scanService::RunUninstall()
{
    rLogger::InitLogging(L"setup");

    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm == NULL)
    {
        const DWORD error = GetLastError();
        ShowMessage(L"Не удалось открыть диспетчер служб (ошибка " + std::to_wstring(error) + L").", true);
        return static_cast<int>(error);
    }

    SC_HANDLE service = OpenServiceW(scm, kServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (service == NULL)
    {
        const DWORD error = GetLastError();
        CloseServiceHandle(scm);
        if (error == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            ShowMessage(L"Служба rus1scan не установлена.", false);
            return 0;
        }
        ShowMessage(L"Не удалось открыть службу rus1scan (ошибка " + std::to_wstring(error) +
                    L"). Запустите rus1scan.exe /uninstall от имени администратора.", true);
        return static_cast<int>(error);
    }

    SERVICE_STATUS status = {};
    if (ControlService(service, SERVICE_CONTROL_STOP, &status))
    {
        const ULONGLONG deadline = GetTickCount64() + 30000;
        while (QueryServiceStatus(service, &status) && status.dwCurrentState != SERVICE_STOPPED &&
               GetTickCount64() < deadline)
        {
            Sleep(250);
        }
        LOG_SAVE << "Service stop requested, state " << status.dwCurrentState;
    }

    const BOOL deleted = DeleteService(service);
    const DWORD error = deleted ? NO_ERROR : GetLastError();
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    if (!deleted)
    {
        ShowMessage(L"Не удалось удалить службу rus1scan (ошибка " + std::to_wstring(error) + L").", true);
        return static_cast<int>(error);
    }
    LOG_SAVE << "Service removed";
    ShowMessage(L"Служба rus1scan остановлена и удалена.", false);
    return 0;
}

// Режим запуска самой WIA по событию-"действию" (постоянная регистрация, которую делает
// служба): rus1scan.exe /StiDevice:<ID устройства> /StiEvent:<GUID события>.
// Работает без окон: читает конфиг, находит привязку, сканирует и выполняет действие.
//   deviceId  - ID устройства, подставленный WIA;
//   eventGuid - GUID события, подставленный WIA.
// Возвращает 0 при успехе.
int CRus1scanService::RunStiLaunch(const std::wstring& deviceId, const std::wstring& eventGuid)
{
    rLogger::InitLogging(L"sti");
    LOG_SAVE << "Launched by WIA: device=" << deviceId << " event=" << eventGuid;

    GUID guid;
    if (deviceId.empty() || !strutil::StringToGuid(eventGuid, guid))
    {
        LOG_SAVE << "Invalid /StiDevice or /StiEvent parameter";
        return 2;
    }

    Rus1Config config;
    std::wstring error;
    if (!LoadConfig(GetConfigFilePath(), config, error))
    {
        LOG_SAVE << "Config error: " << error;
        return 3;
    }

    const EventBinding* binding = config.FindForEvent(deviceId, strutil::GuidToString(guid));
    if (binding == nullptr)
    {
        LOG_SAVE << "No binding for this event";
        return 0;
    }

    const HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    const bool ok = actions::ExecuteBinding(*binding, config, deviceId, std::wstring());
    if (SUCCEEDED(hr))
    {
        CoUninitialize();
    }
    return ok ? 0 : 1;
}

// Режим /process <файл.bmp> <GUID события> [ID устройства]: выполняет действие привязки
// над готовым BMP-файлом, как если бы он был только что отсканирован. Позволяет
// проверить сохранение/PDF/печать/запуск программы без сканера.
//   args - параметры командной строки: args[0] = "/process", далее файл, GUID и,
//          необязательно, ID устройства.
// Возвращает 0 при успехе.
int CRus1scanService::RunProcessFile(const std::vector<std::wstring>& args)
{
    rLogger::InitLogging(L"process", HasStdOutput());

    if (args.size() < 3)
    {
        ShowMessage(L"Использование: rus1scan.exe /process <файл.bmp> <GUID события> [ID устройства]", true);
        return 2;
    }

    GUID guid;
    if (!strutil::StringToGuid(args[2], guid))
    {
        ShowMessage(L"Неверный GUID события: " + args[2], true);
        return 2;
    }
    const std::wstring deviceId = args.size() > 3 ? args[3] : std::wstring();

    Rus1Config config;
    std::wstring error;
    if (!LoadConfig(GetConfigFilePath(), config, error))
    {
        ShowMessage(L"Ошибка в " + GetConfigFilePath() + L": " + error, true);
        return 3;
    }

    const EventBinding* binding = config.FindForEvent(deviceId, strutil::GuidToString(guid));
    if (binding == nullptr)
    {
        ShowMessage(L"В конфиге нет привязки для события " + strutil::GuidToString(guid), true);
        return 4;
    }

    ScannedImage image;
    if (!actions::LoadBmpFile(args[1], image, error))
    {
        ShowMessage(error, true);
        return 5;
    }

    const HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    const bool ok = actions::RunActionOnImages(*binding, config, std::vector<ScannedImage>{ image });
    if (SUCCEEDED(hr))
    {
        CoUninitialize();
    }
    LOG_SAVE << "Process result: " << (ok ? "success" : "failed");
    return ok ? 0 : 1;
}

// Показывает сообщение пользователю: в стандартный вывод, если он есть (запуск из
// консоли/скрипта), иначе окном MessageBox (если не указан /quiet). Всегда пишет в журнал.
//   text    - текст сообщения;
//   isError - true для ошибки (значок ошибки в окне).
void CRus1scanService::ShowMessage(const std::wstring& text, bool isError)
{
    LOG_SAVE << (isError ? "Error: " : "") << text;
    if (HasStdOutput())
    {
        WriteStdOut(text);
    }
    else if (!m_quiet)
    {
        MessageBoxW(NULL, text.c_str(), L"rus1scan", MB_OK | (isError ? MB_ICONERROR : MB_ICONINFORMATION));
    }
}
