// rus1scanWindow.cpp: окно настроек rus1scan (см. rus1scanWindow.h).

#include "stdafx.h"

#include "rus1scanWindow.h"
#include "actions.h"
#include "strutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    // Текст первого пункта списка принтеров: печать на принтер по умолчанию.
    const wchar_t* const kDefaultPrinterItem = L"(default printer)";

    // Текст флажка под списком устройств.
    const wchar_t* const kShowDisconnectedText = L"Показывать в списке отключенные устройства";

    // Добавляет событие в список, если такого GUID там ещё нет; если есть -
    // объединяет флаги и дополняет пустые имя/описание.
    //   events - список событий;
    //   event  - добавляемое событие.
    void MergeEvent(std::vector<WiaEventInfo>& events, const WiaEventInfo& event)
    {
        for (auto& existing : events)
        {
            if (strutil::EqualsNoCase(existing.guid, event.guid))
            {
                existing.flags |= event.flags;
                if (existing.name.empty()) existing.name = event.name;
                if (existing.description.empty()) existing.description = event.description;
                return;
            }
        }
        events.push_back(event);
    }

    // Возвращает текстовое описание флагов события для строки "Description".
    //   flags - WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT.
    std::wstring DescribeEventFlags(ULONG flags)
    {
        if ((flags & WIA_NOTIFICATION_EVENT) && (flags & WIA_ACTION_EVENT))
            return L"notification + action event (handled by the service)";
        if (flags & WIA_NOTIFICATION_EVENT)
            return L"notification event (handled by the service)";
        if (flags & WIA_ACTION_EVENT)
            return L"action event (WIA starts rus1scan.exe; registered by the service)";
        return L"event type unknown";
    }
}

BEGIN_MESSAGE_MAP(CRus1scanWindow, CDialog)
    ON_WM_SYSCOMMAND()
    ON_CBN_SELCHANGE(IDC_COMBO_DEVICE, &CRus1scanWindow::OnDeviceChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_EVENT, &CRus1scanWindow::OnEventChanged)
    ON_BN_CLICKED(IDC_CHECK_SHOW_DISCONNECTED, &CRus1scanWindow::OnShowDisconnectedChanged)
    ON_BN_CLICKED(IDC_RADIO_PRINT, &CRus1scanWindow::OnActionChanged)
    ON_BN_CLICKED(IDC_RADIO_START, &CRus1scanWindow::OnActionChanged)
    ON_BN_CLICKED(IDC_RADIO_NOTHING, &CRus1scanWindow::OnActionChanged)
    ON_BN_CLICKED(IDC_RADIO_SAVEFOLDER, &CRus1scanWindow::OnActionChanged)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CRus1scanWindow::OnBnClickedButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_APPLY, &CRus1scanWindow::OnBnClickedButtonApply)
END_MESSAGE_MAP()

// Создаёт окно настроек.
//   pParent - родительское окно (NULL - нет).
CRus1scanWindow::CRus1scanWindow(CWnd* pParent)
    : CDialog(CRus1scanWindow::IDD, pParent)
{
}

// Связывает переменные-члены с элементами управления диалога.
//   pDX - контекст обмена данными MFC.
void CRus1scanWindow::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_DEVICE, m_ComboBoxDevices);
    DDX_Control(pDX, IDC_COMBO_EVENT, m_ComboBoxEvents);
    DDX_Control(pDX, IDC_COMBO_PRINTER, m_ComboBoxPrinters);
    DDX_Control(pDX, IDC_COMBO_START, m_ComboBoxStart);
    DDX_Control(pDX, IDC_EDIT_FOLDER, m_editSaveDirectory);
    DDX_Control(pDX, IDC_CHECK_PDF, m_checkPdf);
    DDX_Control(pDX, IDC_CHECK_SHOW_DISCONNECTED, m_checkShowDisconnected);
    DDX_Control(pDX, IDC_STATIC_DESCRIPTION, m_DescrText);
    DDX_Control(pDX, IDC_STATIC_ASADMIN, m_StatusText);
}

// Инициализация окна: значок, пункт "About" в системном меню, подсказки, загрузка
// rus1scan.toml, списки принтеров и устройств WIA, отображение текущей привязки.
// Возвращает TRUE (фокус ставит Windows).
BOOL CRus1scanWindow::OnInitDialog()
{
    CDialog::OnInitDialog();

    HICON icon = AfxGetApp()->LoadIcon(IDI_ICON_MAIN);
    SetIcon(icon, TRUE);
    SetIcon(icon, FALSE);

    CMenu* sysMenu = GetSystemMenu(FALSE);
    if (sysMenu != NULL)
    {
        sysMenu->AppendMenu(MF_SEPARATOR);
        sysMenu->AppendMenu(MF_STRING, IDM_ABOUT, L"&About...");
    }

    if (m_ToolTip.Create(this))
    {
        m_ToolTip.AddTool(&m_editSaveDirectory,
                          L"Folder pattern: %d %m %Y %y %H %M %S are replaced with the scan date/time, "
                          L"environment variables like %USERPROFILE% are expanded.");
        m_ToolTip.AddTool(&m_ComboBoxStart, L"Command line; %1 is replaced with the scanned file path.");
        m_ToolTip.Activate(TRUE);
    }

    CWaitCursor wait;

    m_configPath = GetConfigFilePath();
    std::wstring error;
    if (!LoadConfig(m_configPath, m_config, error))
    {
        m_config = Rus1Config();
        const std::wstring text = L"The settings file contains an error and was not loaded:\n\n" + m_configPath +
                                  L"\n" + error + L"\n\nIf you press OK or Apply, the file will be overwritten.";
        MessageBoxW(text.c_str(), L"rus1scan", MB_OK | MB_ICONWARNING);
    }

    m_ComboBoxPrinters.AddString(kDefaultPrinterItem);
    for (const auto& printer : actions::EnumeratePrinters())
    {
        m_ComboBoxPrinters.AddString(printer.c_str());
    }

    m_checkShowDisconnected.SetWindowTextW(kShowDisconnectedText);
    m_checkShowDisconnected.SetCheck(m_config.showDisconnectedDevices ? BST_CHECKED : BST_UNCHECKED);

    FillProgramCombo();
    BuildDeviceList();
    FillDeviceCombo(std::wstring());
    FillEventCombo();
    LoadBindingToControls();

    if (m_ToolTip.GetSafeHwnd() != NULL)
    {
        const std::wstring tip = L"Файл настроек: " + m_configPath +
                                 L"\nOK / Apply сохраняют настройки в этот файл; служба rus1scan перечитывает его сама.";
        m_ToolTip.SetMaxTipWidth(600);
        m_ToolTip.AddTool(&m_StatusText, tip.c_str());
    }
    SetStatus(L"Настройки: rus1scan.toml (рядом с exe)\n" + GetServiceStateText());
    return TRUE;
}

// Передаёт сообщения мыши элементу подсказок перед обычной обработкой.
//   pMsg - сообщение.
BOOL CRus1scanWindow::PreTranslateMessage(MSG* pMsg)
{
    if (m_ToolTip.GetSafeHwnd() != NULL)
    {
        m_ToolTip.RelayEvent(pMsg);
    }
    return CDialog::PreTranslateMessage(pMsg);
}

// Обработчик системного меню: пункт "About..." показывает информацию о программе,
// остальные команды обрабатываются стандартно.
//   nID    - команда меню;
//   lParam - координаты курсора (для команд мыши).
void CRus1scanWindow::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUT)
    {
        MessageBoxW(L"rus1scan - scanner buttons for Windows (WIA)\n(c) 2009-2026 www.alabuga.dev/rus1scan",
                    L"About rus1scan", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// Кнопка OK: сохраняет настройки и закрывает окно (при ошибке сохранения окно остаётся открытым).
void CRus1scanWindow::OnOK()
{
    if (SaveSettings())
    {
        CDialog::OnOK();
    }
}

// Кнопка Apply: сохраняет настройки, окно остаётся открытым.
void CRus1scanWindow::OnBnClickedButtonApply()
{
    SaveSettings();
}

// Выбрано другое устройство: запоминает настройки предыдущей пары
// "устройство + событие" и показывает события нового устройства.
void CRus1scanWindow::OnDeviceChanged()
{
    StoreControlsToBinding();
    const int selection = m_ComboBoxDevices.GetCurSel();
    m_currentDevice = (selection == CB_ERR) ? -1 : static_cast<int>(m_ComboBoxDevices.GetItemData(selection));
    FillEventCombo();
    LoadBindingToControls();
}

// Выбрано другое событие: запоминает настройки предыдущего события и показывает настройки нового.
void CRus1scanWindow::OnEventChanged()
{
    StoreControlsToBinding();
    const int selection = m_ComboBoxEvents.GetCurSel();
    m_currentEvent = (selection == CB_ERR) ? -1 : selection;
    LoadBindingToControls();
}

// Переключён флажок "Показывать в списке отключенные устройства": запоминает настройки
// текущего события, перестраивает список устройств (по возможности оставляя выбранным
// то же устройство) и запоминает состояние флажка для сохранения в rus1scan.toml ([gui]).
void CRus1scanWindow::OnShowDisconnectedChanged()
{
    StoreControlsToBinding();
    m_config.showDisconnectedDevices = (m_checkShowDisconnected.GetCheck() == BST_CHECKED);

    const WiaDeviceInfo* device = CurrentDevice();
    const std::wstring currentId = (device != nullptr) ? device->id : std::wstring();
    FillDeviceCombo(currentId);
    FillEventCombo();
    LoadBindingToControls();
}

// Переключено действие (радиокнопка): включает/выключает связанные поля.
void CRus1scanWindow::OnActionChanged()
{
    UpdateControlStates();
}

// Кнопка "Browse...": выбор папки для сохранения сканов. Если действие ещё не выбрано,
// автоматически выбирается "Save picture to this folder".
void CRus1scanWindow::OnBnClickedButtonBrowse()
{
    CFolderPickerDialog dialog(NULL, 0, this);
    if (dialog.DoModal() != IDOK)
    {
        return;
    }

    std::wstring folder = dialog.GetPathName().GetString();
    if (!folder.empty() && folder.back() != L'\\')
    {
        folder += L'\\';
    }
    m_editSaveDirectory.SetWindowTextW(folder.c_str());

    if (GetSelectedAction() == Rus1Action::Nothing)
    {
        SetSelectedAction(Rus1Action::Save);
        UpdateControlStates();
    }
}

// Собирает полный список устройств для окна: все установленные устройства WIA
// (подключённые - с событиями от драйвера, отключённые - с событиями из привязок
// конфига) и устройства, известные только по привязкам в rus1scan.toml.
// Какие из них показывать, решает FillDeviceCombo по флажку "Показывать отключенные".
// Привязки к "любому устройству" (пустой deviceId) в окне не показываются, но
// сохраняются в конфиге и продолжают работать в службе.
void CRus1scanWindow::BuildDeviceList()
{
    m_devices.clear();
    const HRESULT hr = CRus1scan::EnumerateDevices(m_devices, true);
    if (FAILED(hr))
    {
        LOG_SAVE << "EnumerateDevices failed: " << rLogger::HrText(hr);
    }

    for (const auto& binding : m_config.bindings)
    {
        if (binding.deviceId.empty())
        {
            continue;
        }

        WiaDeviceInfo* device = nullptr;
        for (auto& existing : m_devices)
        {
            if (strutil::EqualsNoCase(existing.id, binding.deviceId))
            {
                device = &existing;
            }
        }
        if (device == nullptr)
        {
            WiaDeviceInfo offline;
            offline.id = binding.deviceId;
            offline.name = binding.deviceName.empty() ? binding.deviceId : binding.deviceName;
            offline.online = false;
            m_devices.push_back(offline);
            device = &m_devices.back();
        }
        if (!device->online)
        {
            WiaEventInfo event;
            event.guid = binding.eventGuid;
            event.name = binding.eventName;
            event.flags = binding.eventFlags;
            MergeEvent(device->events, event);
        }
    }
}

// Заполняет список устройств: подключённые и, если отмечен флажок "Показывать в списке
// отключенные устройства", отключённые (с пометкой). Выбирает устройство preferredId,
// если оно есть в списке, иначе первое подключённое, иначе первое в списке.
//   preferredId - ID устройства, которое желательно оставить выбранным (может быть пустым).
void CRus1scanWindow::FillDeviceCombo(const std::wstring& preferredId)
{
    const bool showDisconnected = (m_checkShowDisconnected.GetCheck() == BST_CHECKED);

    m_ComboBoxDevices.ResetContent();
    int selectIndex = -1;
    int preferredIndex = -1;
    for (size_t i = 0; i < m_devices.size(); ++i)
    {
        const WiaDeviceInfo& device = m_devices[i];
        if (!device.online && !showDisconnected)
        {
            continue;
        }
        std::wstring text = device.name;
        if (!device.online)
        {
            text += L" (отключено)";
        }
        const int index = m_ComboBoxDevices.AddString(text.c_str());
        m_ComboBoxDevices.SetItemData(index, static_cast<DWORD_PTR>(i));
        if (selectIndex < 0 && device.online)
        {
            selectIndex = index;
        }
        if (!preferredId.empty() && strutil::EqualsNoCase(device.id, preferredId))
        {
            preferredIndex = index;
        }
    }
    if (preferredIndex >= 0)
    {
        selectIndex = preferredIndex;
    }
    if (selectIndex < 0 && m_ComboBoxDevices.GetCount() > 0)
    {
        selectIndex = 0;
    }

    m_ComboBoxDevices.SetCurSel(selectIndex);
    m_currentDevice = (selectIndex < 0) ? -1 : static_cast<int>(m_ComboBoxDevices.GetItemData(selectIndex));
}

// Заполняет список событий текущего устройства; выбирает первое событие,
// для которого уже настроено действие (иначе - первое в списке).
void CRus1scanWindow::FillEventCombo()
{
    m_ComboBoxEvents.ResetContent();
    m_currentEvent = -1;

    const WiaDeviceInfo* device = CurrentDevice();
    if (device == nullptr)
    {
        return;
    }

    int configured = -1;
    for (size_t i = 0; i < device->events.size(); ++i)
    {
        const WiaEventInfo& event = device->events[i];
        const std::wstring text = event.name.empty() ? event.guid : event.name;
        m_ComboBoxEvents.AddString(text.c_str());
        if (configured < 0 && m_config.FindExact(device->id, event.guid) != nullptr)
        {
            configured = static_cast<int>(i);
        }
    }

    if (!device->events.empty())
    {
        m_currentEvent = (configured >= 0) ? configured : 0;
        m_ComboBoxEvents.SetCurSel(m_currentEvent);
    }
}

// Заполняет выпадающий список программ: программы из существующих привязок и
// несколько готовых примеров.
void CRus1scanWindow::FillProgramCombo()
{
    m_ComboBoxStart.ResetContent();
    std::vector<std::wstring> programs;
    for (const auto& binding : m_config.bindings)
    {
        if (!binding.program.empty())
        {
            programs.push_back(binding.program);
        }
    }
    programs.push_back(L"mspaint.exe \"%1\"");
    programs.push_back(L"explorer.exe /select,\"%1\"");

    for (const auto& program : programs)
    {
        if (m_ComboBoxStart.FindStringExact(-1, program.c_str()) == CB_ERR)
        {
            m_ComboBoxStart.AddString(program.c_str());
        }
    }
}

// Возвращает выбранное устройство или nullptr.
const WiaDeviceInfo* CRus1scanWindow::CurrentDevice() const
{
    if (m_currentDevice < 0 || m_currentDevice >= static_cast<int>(m_devices.size()))
    {
        return nullptr;
    }
    return &m_devices[m_currentDevice];
}

// Возвращает выбранное событие текущего устройства или nullptr.
const WiaEventInfo* CRus1scanWindow::CurrentEvent() const
{
    const WiaDeviceInfo* device = CurrentDevice();
    if (device == nullptr || m_currentEvent < 0 || m_currentEvent >= static_cast<int>(device->events.size()))
    {
        return nullptr;
    }
    return &device->events[m_currentEvent];
}

// Показывает в элементах окна привязку для выбранной пары "устройство + событие"
// (или значения по умолчанию, если привязки нет).
void CRus1scanWindow::LoadBindingToControls()
{
    const WiaDeviceInfo* device = CurrentDevice();
    const WiaEventInfo* event = CurrentEvent();
    const EventBinding* binding = (device != nullptr && event != nullptr)
                                      ? m_config.FindExact(device->id, event->guid) : nullptr;

    SetSelectedAction(binding != nullptr ? binding->action : Rus1Action::Nothing);

    int printerIndex = 0;
    if (binding != nullptr && !binding->printer.empty())
    {
        printerIndex = m_ComboBoxPrinters.FindStringExact(-1, binding->printer.c_str());
        if (printerIndex == CB_ERR)
        {
            printerIndex = m_ComboBoxPrinters.AddString(binding->printer.c_str());   // Принтер сейчас недоступен
        }
    }
    m_ComboBoxPrinters.SetCurSel(printerIndex);

    m_ComboBoxStart.SetWindowTextW(binding != nullptr ? binding->program.c_str() : L"");
    m_editSaveDirectory.SetWindowTextW((binding != nullptr && !binding->folder.empty())
                                           ? binding->folder.c_str() : kDefaultScanFolder);
    m_checkPdf.SetCheck((binding != nullptr && binding->pdf) ? BST_CHECKED : BST_UNCHECKED);

    std::wstring description;
    if (event != nullptr)
    {
        description = event->description.empty() ? event->guid : event->description;
        description += L" - " + DescribeEventFlags(event->flags);
    }
    else
    {
        description = L"No events for this device.";
    }
    m_DescrText.SetWindowTextW(description.c_str());

    UpdateControlStates();
}

// Переносит значения элементов окна в привязку текущей пары "устройство + событие"
// в m_config. Действие "Take no action" удаляет привязку.
void CRus1scanWindow::StoreControlsToBinding()
{
    const WiaDeviceInfo* device = CurrentDevice();
    const WiaEventInfo* event = CurrentEvent();
    if (device == nullptr || event == nullptr)
    {
        return;
    }

    const Rus1Action action = GetSelectedAction();
    EventBinding* binding = m_config.FindExact(device->id, event->guid);

    if (action == Rus1Action::Nothing)
    {
        if (binding != nullptr)
        {
            const size_t index = static_cast<size_t>(binding - m_config.bindings.data());
            m_config.bindings.erase(m_config.bindings.begin() + static_cast<ptrdiff_t>(index));
        }
        return;
    }

    if (binding == nullptr)
    {
        m_config.bindings.push_back(EventBinding());
        binding = &m_config.bindings.back();
    }

    binding->deviceId = device->id;
    binding->deviceName = device->id.empty() ? std::wstring() : device->name;
    binding->eventGuid = event->guid;
    binding->eventName = event->name;
    if (event->flags != 0)
    {
        binding->eventFlags = event->flags;
    }
    binding->action = action;

    CString text;
    const int printerIndex = m_ComboBoxPrinters.GetCurSel();
    if (printerIndex > 0)
    {
        m_ComboBoxPrinters.GetLBText(printerIndex, text);
        binding->printer = text.GetString();
    }
    else
    {
        binding->printer.clear();       // "(default printer)"
    }

    m_ComboBoxStart.GetWindowTextW(text);
    binding->program = strutil::Trim(text.GetString());

    m_editSaveDirectory.GetWindowTextW(text);
    binding->folder = strutil::Trim(text.GetString());

    binding->pdf = (m_checkPdf.GetCheck() == BST_CHECKED);
}

// Возвращает действие, выбранное радиокнопками.
Rus1Action CRus1scanWindow::GetSelectedAction() const
{
    if (IsDlgButtonChecked(IDC_RADIO_PRINT))      return Rus1Action::Print;
    if (IsDlgButtonChecked(IDC_RADIO_START))      return Rus1Action::Start;
    if (IsDlgButtonChecked(IDC_RADIO_SAVEFOLDER)) return Rus1Action::Save;
    return Rus1Action::Nothing;
}

// Отмечает радиокнопку, соответствующую действию.
//   action - действие.
void CRus1scanWindow::SetSelectedAction(Rus1Action action)
{
    CheckDlgButton(IDC_RADIO_PRINT, action == Rus1Action::Print ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_RADIO_START, action == Rus1Action::Start ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_RADIO_NOTHING, action == Rus1Action::Nothing ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_RADIO_SAVEFOLDER, action == Rus1Action::Save ? BST_CHECKED : BST_UNCHECKED);
}

// Включает только те поля, которые относятся к выбранному действию; если не выбрано
// событие - выключает все элементы действий.
void CRus1scanWindow::UpdateControlStates()
{
    const bool hasEvent = (CurrentEvent() != nullptr);
    const Rus1Action action = GetSelectedAction();
    const bool usesFolder = hasEvent && (action == Rus1Action::Save || action == Rus1Action::Start);

    // Если элемент с фокусом сейчас будет выключен, диалог сам переведёт фокус на
    // следующую радиокнопку, а автоматическая радиокнопка при получении фокуса
    // отмечается - действие незаметно сменилось бы. Поэтому заранее переносим фокус
    // на отмеченную радиокнопку (или на список событий).
    CWnd* focus = GetFocus();
    if (focus != NULL)
    {
        CWnd* control = focus;
        if (control->GetParent() != NULL && control->GetParent() != this)
        {
            control = control->GetParent();     // Поле ввода внутри комбобокса
        }
        const int id = control->GetDlgCtrlID();
        const bool willBeDisabled =
            (id == IDC_COMBO_PRINTER && !(hasEvent && action == Rus1Action::Print)) ||
            (id == IDC_COMBO_START && !(hasEvent && action == Rus1Action::Start)) ||
            ((id == IDC_EDIT_FOLDER || id == IDC_BUTTON_BROWSE || id == IDC_CHECK_PDF) && !usesFolder) ||
            ((id == IDC_RADIO_PRINT || id == IDC_RADIO_START || id == IDC_RADIO_NOTHING || id == IDC_RADIO_SAVEFOLDER) && !hasEvent);
        if (willBeDisabled)
        {
            const int checkedRadio = action == Rus1Action::Print ? IDC_RADIO_PRINT
                                   : action == Rus1Action::Start ? IDC_RADIO_START
                                   : action == Rus1Action::Save  ? IDC_RADIO_SAVEFOLDER
                                   : IDC_RADIO_NOTHING;
            GotoDlgCtrl(hasEvent ? GetDlgItem(checkedRadio) : static_cast<CWnd*>(&m_ComboBoxDevices));
        }
    }

    GetDlgItem(IDC_RADIO_PRINT)->EnableWindow(hasEvent);
    GetDlgItem(IDC_RADIO_START)->EnableWindow(hasEvent);
    GetDlgItem(IDC_RADIO_NOTHING)->EnableWindow(hasEvent);
    GetDlgItem(IDC_RADIO_SAVEFOLDER)->EnableWindow(hasEvent);

    m_ComboBoxPrinters.EnableWindow(hasEvent && action == Rus1Action::Print);
    m_ComboBoxStart.EnableWindow(hasEvent && action == Rus1Action::Start);

    m_editSaveDirectory.EnableWindow(usesFolder);
    GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(usesFolder);
    m_checkPdf.EnableWindow(usesFolder);
}

// Сохраняет настройки окна в rus1scan.toml. При ошибке (например, нет прав на запись
// в папку программы) показывает сообщение.
// Возвращает true при успешном сохранении.
bool CRus1scanWindow::SaveSettings()
{
    StoreControlsToBinding();

    const WiaDeviceInfo* device = CurrentDevice();
    const WiaEventInfo* event = CurrentEvent();
    const EventBinding* binding = (device != nullptr && event != nullptr)
                                      ? m_config.FindExact(device->id, event->guid) : nullptr;
    if (binding != nullptr && binding->action == Rus1Action::Start && binding->program.empty())
    {
        MessageBoxW(L"Enter the program to start.", L"rus1scan", MB_OK | MB_ICONWARNING);
        m_ComboBoxStart.SetFocus();
        return false;
    }

    std::wstring error;
    if (!SaveConfig(m_configPath, m_config, error))
    {
        const std::wstring text = L"Settings were not saved:\n\n" + error;
        MessageBoxW(text.c_str(), L"rus1scan", MB_OK | MB_ICONERROR);
        SetStatus(L"Настройки НЕ сохранены (см. сообщение)\n" + GetServiceStateText());
        return false;
    }

    SYSTEMTIME now;
    GetLocalTime(&now);
    wchar_t stamp[16];
    swprintf_s(stamp, L"%02u:%02u:%02u", now.wHour, now.wMinute, now.wSecond);
    SetStatus(std::wstring(L"Сохранено в rus1scan.toml в ") + stamp + L"\n" + GetServiceStateText());
    return true;
}

// Выводит текст в строку состояния внизу окна.
//   text - текст (может содержать перевод строки).
void CRus1scanWindow::SetStatus(const std::wstring& text)
{
    m_StatusText.SetWindowTextW(text.c_str());
}

// Возвращает описание состояния службы rus1scan для строки состояния.
std::wstring CRus1scanWindow::GetServiceStateText() const
{
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm == NULL)
    {
        return L"Служба: состояние неизвестно";
    }

    std::wstring text;
    SC_HANDLE service = OpenServiceW(scm, L"rus1scan", SERVICE_QUERY_STATUS);
    if (service == NULL)
    {
        text = (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
                   ? L"Служба: не установлена (rus1scan.exe /install)"
                   : L"Служба: состояние неизвестно";
    }
    else
    {
        SERVICE_STATUS status = {};
        if (QueryServiceStatus(service, &status) && status.dwCurrentState == SERVICE_RUNNING)
        {
            text = L"Служба: работает, подхватит изменения сама";
        }
        else
        {
            text = L"Служба: установлена, но остановлена";
        }
        CloseServiceHandle(service);
    }
    CloseServiceHandle(scm);
    return text;
}
