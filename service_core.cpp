// service_core.cpp: рабочая часть службы rus1scan (см. service_core.h).

#include "stdafx.h"
#include "service_core.h"
#include "actions.h"
#include "rus1scan.h"
#include "strutil.h"
#include "WiaWrap.h"

namespace
{
    // Через сколько миллисекунд повтор того же события от того же устройства считается
    // отдельным нажатием (некоторые драйверы присылают одно нажатие дважды).
    const ULONGLONG kDuplicateEventWindowMs = 2000;

    // Возвращает командную строку, которую WIA выполнит для постоянно зарегистрированного
    // события: "<путь к exe>" /StiDevice:%1 /StiEvent:%2 (WIA подставит ID устройства и GUID события).
    std::wstring GetStiCommandLine()
    {
        return L"\"" + strutil::GetModulePath() + L"\" /StiDevice:%1 /StiEvent:%2";
    }
}

// ---------------------------------------------------------------------------
// CEventQueue

// Создаёт пустую очередь и событие Win32 с автосбросом для пробуждения рабочего потока.
CEventQueue::CEventQueue()
{
    m_signal = CreateEventW(NULL, FALSE, FALSE, NULL);
}

// Закрывает событие Win32.
CEventQueue::~CEventQueue()
{
    if (m_signal != NULL)
    {
        CloseHandle(m_signal);
    }
}

// Добавляет событие в конец очереди и взводит сигнал. Может вызываться из любого потока.
//   event - копия данных события WIA.
void CEventQueue::Push(const WiaEventNotification& event)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_items.push_back(event);
    }
    SetEvent(m_signal);
}

// Забирает первое событие из очереди.
//   event - [out] событие.
// Возвращает false, если очередь пуста.
bool CEventQueue::Pop(WiaEventNotification& event)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_items.empty())
    {
        return false;
    }
    event = m_items.front();
    m_items.pop_front();
    return true;
}

// ---------------------------------------------------------------------------
// CServiceCore

// Создаёт ядро службы.
//   stopEvent - событие Win32 (ручной сброс), взведение которого останавливает рабочий поток.
//               Принадлежит вызывающему и должно жить дольше объекта.
CServiceCore::CServiceCore(HANDLE stopEvent)
    : m_stopEvent(stopEvent), m_queue(std::make_shared<CEventQueue>())
{
}

// Дожидается рабочего потока (если он запущен) и закрывает его дескриптор.
CServiceCore::~CServiceCore()
{
    if (m_thread != NULL)
    {
        WaitForSingleObject(m_thread, INFINITE);
        CloseHandle(m_thread);
    }
}

// Запускает рабочий поток службы.
// Возвращает false, если поток создать не удалось.
bool CServiceCore::Start()
{
    m_thread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
    if (m_thread == NULL)
    {
        LOG_SAVE << "CreateThread failed, error " << GetLastError();
        m_exitCode = GetLastError();
        return false;
    }
    return true;
}

// Ждёт завершения рабочего потока (после взведения stopEvent).
void CServiceCore::Wait()
{
    if (m_thread != NULL)
    {
        WaitForSingleObject(m_thread, INFINITE);
    }
}

// Точка входа рабочего потока.
//   param - указатель на CServiceCore.
DWORD WINAPI CServiceCore::ThreadProc(LPVOID param)
{
    CServiceCore* self = static_cast<CServiceCore*>(param);
    self->m_exitCode = self->Run();
    return self->m_exitCode;
}

// Основной цикл службы: инициализация COM и WIA, затем ожидание одного из сигналов -
// остановка, новое событие WIA в очереди или изменение файлов в папке программы
// (перечитывание rus1scan.toml). При остановке отменяет все регистрации в WIA.
// Возвращает код завершения (0 - успех).
DWORD CServiceCore::Run()
{
    LOG_SAVE << "Worker thread started";

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        LOG_SAVE << "CoInitializeEx failed: " << rLogger::HrText(hr);
        return static_cast<DWORD>(hr);
    }

    hr = InitializeComSecurityForService();
    LOG_SAVE << "COM security: " << rLogger::HrText(hr);

    m_configPath = GetConfigFilePath();
    LOG_SAVE << "Config file: " << m_configPath;
    ReloadConfig(true);

    HANDLE dirWatch = FindFirstChangeNotificationW(strutil::GetModuleDir().c_str(), FALSE,
                                                   FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME |
                                                   FILE_NOTIFY_CHANGE_SIZE);
    if (dirWatch == INVALID_HANDLE_VALUE)
    {
        LOG_SAVE << "FindFirstChangeNotification failed, error " << GetLastError()
                 << " - config changes require service restart";
    }

    for (;;)
    {
        HANDLE handles[3] = { m_stopEvent, m_queue->SignalHandle(), dirWatch };
        const DWORD count = (dirWatch != INVALID_HANDLE_VALUE) ? 3 : 2;
        const DWORD wait = WaitForMultipleObjects(count, handles, FALSE, INFINITE);

        if (wait == WAIT_OBJECT_0)
        {
            LOG_SAVE << "Stop requested";
            break;
        }
        if (wait == WAIT_OBJECT_0 + 1)
        {
            ProcessQueue();
        }
        else if (wait == WAIT_OBJECT_0 + 2)
        {
            FindNextChangeNotification(dirWatch);

            // Здесь нельзя писать в журнал без изменения конфига: запись в logs\ сама
            // по себе меняет содержимое папки и снова разбудила бы этот цикл.
            FILETIME time = {};
            if (GetConfigWriteTime(time) && CompareFileTime(&time, &m_configTime) != 0)
            {
                // Даём окну настроек дописать файл, затем перечитываем
                if (WaitForSingleObject(m_stopEvent, 500) == WAIT_OBJECT_0)
                {
                    LOG_SAVE << "Stop requested";
                    break;
                }
                LOG_SAVE << "Config file changed, reloading";
                ReloadConfig(false);
            }
        }
        else
        {
            LOG_SAVE << "WaitForMultipleObjects failed, error " << GetLastError();
            if (WaitForSingleObject(m_stopEvent, 1000) == WAIT_OBJECT_0)
            {
                break;
            }
        }
    }

    UnregisterEvents();
    UnregisterPersistentHandlers();
    m_manager.Release();

    if (dirWatch != INVALID_HANDLE_VALUE)
    {
        FindCloseChangeNotification(dirWatch);
    }

    CoUninitialize();
    LOG_SAVE << "Worker thread finished";
    return 0;
}

// Возвращает время последнего изменения rus1scan.toml.
//   time - [out] время изменения.
// Возвращает false, если файла нет.
bool CServiceCore::GetConfigWriteTime(FILETIME& time) const
{
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (!GetFileAttributesExW(m_configPath.c_str(), GetFileExInfoStandard, &data))
    {
        return false;
    }
    time = data.ftLastWriteTime;
    return true;
}

// Читает конфиг и заново регистрирует события в WIA.
//   initial - true при старте службы: даже при ошибке в конфиге регистрируются
//             события подключения/отключения; при перечитывании (false) ошибка в
//             конфиге оставляет в силе предыдущие настройки.
// Возвращает true, если конфиг прочитан успешно.
bool CServiceCore::ReloadConfig(bool initial)
{
    FILETIME time = {};
    GetConfigWriteTime(time);

    Rus1Config config;
    std::wstring error;
    const bool loaded = LoadConfig(m_configPath, config, error);
    m_configTime = time;

    if (!loaded)
    {
        LOG_SAVE << "Config error: " << error << (initial ? L" - starting without bindings" : L" - keeping previous settings");
        if (!initial)
        {
            return false;
        }
        config = Rus1Config();
    }

    m_config = config;
    LOG_SAVE << "Config loaded: " << m_config.bindings.size() << " binding(s), " << m_config.wiaProps.size()
             << " WIA propert(ies)";
    for (const auto& b : m_config.bindings)
    {
        LOG_SAVE << "  binding: device='" << (b.deviceId.empty() ? std::wstring(L"<any>") : b.deviceId) << "' event="
                 << b.eventGuid << " '" << b.eventName << "' flags=" << b.eventFlags << " action=" << ActionToString(b.action);
    }

    UnregisterEvents();
    UnregisterPersistentHandlers();
    RegisterEvents();
    RegisterPersistentHandlers();
    return loaded;
}

// Регистрирует приёмник событий WIA (RegisterEventCallbackInterface) на события
// подключения/отключения устройств и на все события из привязок с действием.
void CServiceCore::RegisterEvents()
{
    if (m_manager == NULL)
    {
        if (FAILED(CRus1scan::CreateDeviceManager(m_manager)))
        {
            LOG_SAVE << "WIA is not available - events are not registered";
            return;
        }
        ULONG devices = 0;
        WiaWrap::WiaGetNumDevices(m_manager, &devices);
        LOG_SAVE << "Connected to WIA, devices: " << devices;
    }

    std::vector<GUID> guids = { WIA_EVENT_DEVICE_CONNECTED, WIA_EVENT_DEVICE_DISCONNECTED };
    for (const auto& binding : m_config.bindings)
    {
        GUID guid;
        if (binding.action == Rus1Action::Nothing || !strutil::StringToGuid(binding.eventGuid, guid))
        {
            continue;
        }
        bool known = false;
        for (const GUID& existing : guids)
        {
            known = known || IsEqualGUID(existing, guid);
        }
        if (!known)
        {
            guids.push_back(guid);
        }
    }

    // Очередь захватывается через shared_ptr: если WIA пришлёт запоздалое событие
    // после остановки ядра, обработчик обратится к ещё живой очереди.
    std::shared_ptr<CEventQueue> queue = m_queue;
    m_callback = new WiaWrap::CEventCallback([queue](const WiaEventNotification& event) { queue->Push(event); });

    const HRESULT hr = m_callback->Register(m_manager, guids);
    LOG_SAVE << "Runtime WIA registrations: " << m_callback->RegisteredCount() << " of " << guids.size()
             << " (" << rLogger::HrText(hr) << ")";
}

// Отменяет регистрации RegisterEventCallbackInterface.
void CServiceCore::UnregisterEvents()
{
    if (m_callback != NULL)
    {
        m_callback->Unregister();
        m_callback.Release();
    }
}

// Для привязок к событиям, которые устройство отдаёт только как "действие"
// (eventFlags содержит WIA_ACTION_EVENT без WIA_NOTIFICATION_EVENT), регистрирует
// постоянный обработчик RegisterEventCallbackProgram и делает его обработчиком по
// умолчанию. Такие события не приходят в RegisterEventCallbackInterface - WIA сама
// запускает "rus1scan.exe /StiDevice:... /StiEvent:..." в сеансе пользователя.
void CServiceCore::RegisterPersistentHandlers()
{
    if (m_manager == NULL)
    {
        return;
    }

    const std::wstring exe = strutil::GetModulePath();
    CComBSTR commandLine(GetStiCommandLine().c_str());
    CComBSTR name(L"rus1scan");
    CComBSTR icon((exe + L",0").c_str());

    for (const auto& binding : m_config.bindings)
    {
        GUID guid;
        if (binding.action == Rus1Action::Nothing || !(binding.eventFlags & WIA_ACTION_EVENT) ||
            (binding.eventFlags & WIA_NOTIFICATION_EVENT) || !strutil::StringToGuid(binding.eventGuid, guid))
        {
            continue;
        }

        CComBSTR device(binding.deviceId.c_str());
        CComBSTR description((L"rus1scan: " + ActionToString(binding.action)).c_str());
        const HRESULT hr = m_manager->RegisterEventCallbackProgram(
            WIA_REGISTER_EVENT_CALLBACK | WIA_SET_DEFAULT_HANDLER,
            binding.deviceId.empty() ? NULL : device.m_str, &guid, commandLine, name, description, icon);

        LOG_SAVE << "Persistent handler for " << binding.eventGuid << " on '"
                 << (binding.deviceId.empty() ? std::wstring(L"<any>") : binding.deviceId) << "': " << rLogger::HrText(hr);
        if (SUCCEEDED(hr))
        {
            m_persistent.push_back({ binding.deviceId, guid });
        }
    }
}

// Отменяет постоянные обработчики, зарегистрированные RegisterPersistentHandlers,
// чтобы после остановки службы WIA больше не запускала rus1scan.exe.
void CServiceCore::UnregisterPersistentHandlers()
{
    if (m_manager == NULL || m_persistent.empty())
    {
        m_persistent.clear();
        return;
    }

    const std::wstring exe = strutil::GetModulePath();
    CComBSTR commandLine(GetStiCommandLine().c_str());
    CComBSTR name(L"rus1scan");
    CComBSTR icon((exe + L",0").c_str());

    for (const auto& reg : m_persistent)
    {
        CComBSTR device(reg.deviceId.c_str());
        CComBSTR description(L"rus1scan");
        const HRESULT hr = m_manager->RegisterEventCallbackProgram(
            WIA_UNREGISTER_EVENT_CALLBACK, reg.deviceId.empty() ? NULL : device.m_str, &reg.eventGuid,
            commandLine, name, description, icon);
        LOG_SAVE << "Persistent handler removed for " << strutil::GuidToString(reg.eventGuid) << ": " << rLogger::HrText(hr);
    }
    m_persistent.clear();
}

// Обрабатывает все события из очереди по одному (сканирование выполняется
// последовательно). Прерывается, если запрошена остановка.
void CServiceCore::ProcessQueue()
{
    WiaEventNotification event;
    while (m_queue->Pop(event))
    {
        if (WaitForSingleObject(m_stopEvent, 0) == WAIT_OBJECT_0)
        {
            LOG_SAVE << "Stop requested - remaining events dropped";
            return;
        }
        HandleEvent(event);
    }
}

// Обрабатывает одно событие WIA: подключение/отключение только пишется в журнал,
// для остальных ищется привязка и выполняется сканирование с действием.
//   event - событие из очереди.
void CServiceCore::HandleEvent(const WiaEventNotification& event)
{
    if (IsEqualGUID(event.eventGuid, WIA_EVENT_DEVICE_CONNECTED))
    {
        LOG_SAVE << "Device connected: '" << event.deviceDescription << "' (" << event.deviceId << ")";
        return;
    }
    if (IsEqualGUID(event.eventGuid, WIA_EVENT_DEVICE_DISCONNECTED))
    {
        LOG_SAVE << "Device disconnected: '" << event.deviceDescription << "' (" << event.deviceId << ")";
        return;
    }

    const std::wstring key = event.deviceId + L"|" + event.eventGuidText;
    const ULONGLONG now = GetTickCount64();
    const auto last = m_lastEventTick.find(key);
    if (last != m_lastEventTick.end() && now - last->second < kDuplicateEventWindowMs)
    {
        LOG_SAVE << "Duplicate event within " << kDuplicateEventWindowMs << " ms ignored";
        return;
    }
    m_lastEventTick[key] = now;

    const EventBinding* binding = m_config.FindForEvent(event.deviceId, event.eventGuidText);
    if (binding == nullptr)
    {
        LOG_SAVE << "No binding for event " << event.eventGuidText << " from " << event.deviceId;
        return;
    }

    const bool ok = actions::ExecuteBinding(*binding, m_config, event.deviceId, event.fullItemName);
    LOG_SAVE << "Event processed: " << (ok ? "success" : "failed");
}

// Настраивает безопасность COM процесса. Служба WIA (stisvc) работает от имени
// NT AUTHORITY\LocalService и вызывает наш IWiaEventCallback обратно; при настройках
// по умолчанию процесс службы (LocalSystem) может отклонить такой вызов, и события
// молча не доходят. Разрешаем локальные вызовы от SYSTEM, LocalService, NetworkService,
// администраторов и самого процесса; удалённые вызовы запрещены.
// Возвращает результат CoInitializeSecurity (RPC_E_TOO_LATE, если безопасность уже задана).
HRESULT InitializeComSecurityForService()
{
    // 0x3 = COM_RIGHTS_EXECUTE | COM_RIGHTS_EXECUTE_LOCAL
    const wchar_t* sddl = L"O:BAG:BAD:(A;;0x3;;;SY)(A;;0x3;;;LS)(A;;0x3;;;NS)(A;;0x3;;;BA)(A;;0x3;;;PS)";

    PSECURITY_DESCRIPTOR relative = NULL;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &relative, NULL))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // CoInitializeSecurity требует дескриптор в абсолютном формате; буферы живут до конца процесса
    static std::vector<BYTE> absoluteSd, dacl, sacl, owner, group;
    DWORD absoluteSize = 0, daclSize = 0, saclSize = 0, ownerSize = 0, groupSize = 0;
    MakeAbsoluteSD(relative, NULL, &absoluteSize, NULL, &daclSize, NULL, &saclSize, NULL, &ownerSize, NULL, &groupSize);

    absoluteSd.resize(absoluteSize ? absoluteSize : 1);
    dacl.resize(daclSize ? daclSize : 1);
    sacl.resize(saclSize ? saclSize : 1);
    owner.resize(ownerSize ? ownerSize : 1);
    group.resize(groupSize ? groupSize : 1);

    const BOOL made = MakeAbsoluteSD(relative,
                                     absoluteSd.data(), &absoluteSize,
                                     reinterpret_cast<PACL>(dacl.data()), &daclSize,
                                     reinterpret_cast<PACL>(sacl.data()), &saclSize,
                                     owner.data(), &ownerSize,
                                     group.data(), &groupSize);
    const DWORD makeError = GetLastError();
    LocalFree(relative);
    if (!made)
    {
        return HRESULT_FROM_WIN32(makeError);
    }

    return CoInitializeSecurity(absoluteSd.data(), -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, NULL);
}
