// config.h: настройки rus1scan и их хранение в глобальном конфиге rus1scan.toml.
//
// Конфиг лежит рядом с rus1scan.exe. Его читают служба, режим запуска по событию WIA
// и окно настроек; записывает только окно настроек (кнопки OK / Apply).
// Служба следит за файлом и перечитывает его автоматически после сохранения.

#pragma once

#include <string>
#include <vector>
#include <utility>

// Что сделать с отсканированным изображением при наступлении события.
enum class Rus1Action
{
    Nothing,    // Ничего не делать (событие только пишется в журнал)
    Save,       // Сохранить в папку (BMP или PDF)
    Print,      // Напечатать на принтере
    Start       // Сохранить и запустить программу, передав ей путь к файлу
};

// Один параметр из секции [wia]: WIA-свойство, которое выставляется перед сканированием.
// Ключ - имя из WIA_CONSTANTS.TXT.H (WIA_IPS_XRES), отображаемое имя или номер свойства.
// Значение приводится к типу свойства устройства в момент сканирования (см. wia_names.h).
struct WiaSetting
{
    enum class Kind { Integer, Boolean, String };

    std::wstring  key;              // Ключ как записан в конфиге
    unsigned long propId = 0;       // Номер свойства WIA
    Kind          kind = Kind::Integer;
    long long     integer = 0;      // Kind::Integer
    bool          boolean = false;  // Kind::Boolean
    std::wstring  text;             // Kind::String: константы (FEEDER|DUPLEX), число, GUID или текст
    int           line = 0;         // Строка в rus1scan.toml (для журнала)
};

// Параметры секции [wia] в порядке записи в конфиге.
using WiaPropList = std::vector<WiaSetting>;

// Привязка "устройство + событие (кнопка) -> действие".
struct EventBinding
{
    std::wstring  deviceId;         // WIA_DIP_DEV_ID устройства; пусто = любое устройство
    std::wstring  deviceName;       // Имя устройства (только для удобства чтения конфига)
    std::wstring  eventGuid;        // GUID события WIA в виде "{...}"
    std::wstring  eventName;        // Имя события (только для удобства чтения конфига)
    unsigned long eventFlags = 0;   // WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT; 0 = неизвестно
    Rus1Action    action = Rus1Action::Nothing;
    std::wstring  folder;           // Save/Start: шаблон папки, например C:\Scans\%d.%m.%Y\ ;
    bool          pdf = false;      // Save/Start: сохранять один PDF вместо BMP-файлов
    std::wstring  printer;          // Print: имя принтера; пусто = принтер по умолчанию
    std::wstring  program;          // Start: командная строка программы (%1 = путь к файлу)
};

// Все настройки rus1scan.
struct Rus1Config
{
    std::wstring              filePrefix = L"Scan_";   // Префикс имён файлов сканов
    WiaPropList               wiaProps;                // Секция [wia]
    std::vector<EventBinding> bindings;                // Массив [[binding]]
    bool                      showDisconnectedDevices = false;  // [gui]: показывать отключённые устройства

    // Ищет привязку для пришедшего события (сначала точное устройство, затем "любое").
    const EventBinding* FindForEvent(const std::wstring& deviceId, const std::wstring& eventGuid) const;

    // Ищет привязку с точно таким же deviceId и eventGuid (для окна настроек).
    EventBinding* FindExact(const std::wstring& deviceId, const std::wstring& eventGuid);
};

// Папка по умолчанию для сохранения сканов.
extern const wchar_t* const kDefaultScanFolder;

// Полный путь к глобальному конфигу: <папка exe>\rus1scan.toml.
std::wstring GetConfigFilePath();

// Читает конфиг из файла.
bool LoadConfig(const std::wstring& path, Rus1Config& config, std::wstring& error);

// Записывает конфиг в файл (атомарно, через временный файл).
bool SaveConfig(const std::wstring& path, const Rus1Config& config, std::wstring& error);

// Rus1Action <-> строка в конфиге ("nothing", "save", "print", "start").
std::wstring ActionToString(Rus1Action action);
bool ActionFromString(const std::wstring& text, Rus1Action& action);
