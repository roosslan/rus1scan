// config.cpp: чтение и запись глобального конфига rus1scan.toml (см. config.h).

#include "stdafx.h"
#include "config.h"
#include "strutil.h"
#include "toml_lite.h"
#include "wia_names.h"

#include <fstream>

const wchar_t* const kDefaultScanFolder = L"C:\\Scans\\%d.%m.%Y\\";

namespace
{
    // Проверяет тип значения и достаёт строку.
    //   value - значение из TOML;
    //   key   - имя ключа (для сообщения об ошибке);
    //   out   - [out] строка UTF-16;
    //   error - [out] текст ошибки.
    bool ReadString(const tomllite::Value& value, const std::string& key, std::wstring& out, std::wstring& error)
    {
        if (value.type != tomllite::Value::Type::String)
        {
            error = L"строка " + std::to_wstring(value.line) + L": значение \"" + strutil::Utf8ToWide(key) +
                    L"\" должно быть строкой в кавычках";
            return false;
        }
        out = strutil::Utf8ToWide(value.str);
        return true;
    }

    // Проверяет тип значения и достаёт целое число в заданных пределах.
    //   value    - значение из TOML;
    //   key      - имя ключа (для сообщения об ошибке);
    //   minValue - наименьшее допустимое значение;
    //   maxValue - наибольшее допустимое значение;
    //   out      - [out] число;
    //   error    - [out] текст ошибки.
    bool ReadInteger(const tomllite::Value& value, const std::string& key, long long minValue, long long maxValue,
                     long long& out, std::wstring& error)
    {
        if (value.type != tomllite::Value::Type::Integer)
        {
            error = L"строка " + std::to_wstring(value.line) + L": значение \"" + strutil::Utf8ToWide(key) +
                    L"\" должно быть целым числом";
            return false;
        }
        if (value.integer < minValue || value.integer > maxValue)
        {
            error = L"строка " + std::to_wstring(value.line) + L": значение \"" + strutil::Utf8ToWide(key) +
                    L"\" вне допустимого диапазона";
            return false;
        }
        out = value.integer;
        return true;
    }

    // Проверяет тип значения и достаёт true/false.
    //   value - значение из TOML;
    //   key   - имя ключа (для сообщения об ошибке);
    //   out   - [out] логическое значение;
    //   error - [out] текст ошибки.
    bool ReadBool(const tomllite::Value& value, const std::string& key, bool& out, std::wstring& error)
    {
        if (value.type != tomllite::Value::Type::Boolean)
        {
            error = L"строка " + std::to_wstring(value.line) + L": значение \"" + strutil::Utf8ToWide(key) +
                    L"\" должно быть true или false";
            return false;
        }
        out = value.boolean;
        return true;
    }

    // Разбирает один элемент [[binding]] в структуру EventBinding.
    //   table   - таблица TOML с ключами привязки;
    //   binding - [out] результат;
    //   error   - [out] текст ошибки.
    bool ReadBinding(const tomllite::Table& table, EventBinding& binding, std::wstring& error)
    {
        bool hasGuid = false;
        for (const auto& entry : table.entries)
        {
            const std::string& key = entry.first;
            const tomllite::Value& value = entry.second;

            if (key == "deviceId")
            {
                if (!ReadString(value, key, binding.deviceId, error)) return false;
                binding.deviceId = strutil::Trim(binding.deviceId);
            }
            else if (key == "deviceName")
            {
                if (!ReadString(value, key, binding.deviceName, error)) return false;
            }
            else if (key == "eventGuid")
            {
                std::wstring text;
                GUID guid;
                if (!ReadString(value, key, text, error)) return false;
                if (!strutil::StringToGuid(text, guid))
                {
                    error = L"строка " + std::to_wstring(value.line) + L": eventGuid \"" + text + L"\" не является GUID";
                    return false;
                }
                binding.eventGuid = strutil::GuidToString(guid);
                hasGuid = true;
            }
            else if (key == "eventName")
            {
                if (!ReadString(value, key, binding.eventName, error)) return false;
            }
            else if (key == "eventFlags")
            {
                long long flags = 0;
                if (!ReadInteger(value, key, 0, 0xFFFFFFFFLL, flags, error)) return false;
                binding.eventFlags = static_cast<unsigned long>(flags);
            }
            else if (key == "action")
            {
                std::wstring text;
                if (!ReadString(value, key, text, error)) return false;
                if (!ActionFromString(text, binding.action))
                {
                    error = L"строка " + std::to_wstring(value.line) + L": неизвестное действие \"" + text +
                            L"\" (допустимо: nothing, save, print, start)";
                    return false;
                }
            }
            else if (key == "folder")
            {
                if (!ReadString(value, key, binding.folder, error)) return false;
            }
            else if (key == "pdf")
            {
                if (!ReadBool(value, key, binding.pdf, error)) return false;
            }
            else if (key == "printer")
            {
                if (!ReadString(value, key, binding.printer, error)) return false;
            }
            else if (key == "program")
            {
                if (!ReadString(value, key, binding.program, error)) return false;
            }
            else
            {
                LOG_SAVE << "rus1scan.toml line " << value.line << ": unknown key '" << key << "' in [[binding]] ignored";
            }
        }

        if (!hasGuid)
        {
            error = L"строка " + std::to_wstring(table.line) + L": в [[binding]] не указан eventGuid";
            return false;
        }
        return true;
    }

    // Разбирает одну строку секции [wia]: ключ - имя/номер WIA-свойства, значение -
    // число, true/false или строка (константы через |, число, GUID, текст).
    //   key      - ключ из TOML (UTF-8);
    //   value    - значение из TOML;
    //   settings - [in/out] список параметров, в который добавляется новый;
    //   error    - [out] текст ошибки (неизвестное свойство, повтор, неверное число).
    bool ReadWiaSetting(const std::string& key, const tomllite::Value& value, WiaPropList& settings, std::wstring& error)
    {
        WiaSetting setting;
        setting.key = strutil::Utf8ToWide(key);
        setting.line = value.line;

        if (!wianames::FindPropertyId(setting.key, setting.propId))
        {
            error = L"строка " + std::to_wstring(value.line) + L": неизвестный WIA-параметр \"" + setting.key +
                    L"\" (имена - в WIA_CONSTANTS.TXT.H, например WIA_IPS_XRES)";
            return false;
        }
        for (const auto& existing : settings)
        {
            if (existing.propId == setting.propId)
            {
                error = L"строка " + std::to_wstring(value.line) + L": WIA-параметр \"" + setting.key +
                        L"\" уже задан в строке " + std::to_wstring(existing.line);
                return false;
            }
        }

        switch (value.type)
        {
        case tomllite::Value::Type::Integer:
            if (!ReadInteger(value, key, LONG_MIN, 0xFFFFFFFFLL, setting.integer, error)) return false;
            setting.kind = WiaSetting::Kind::Integer;
            break;
        case tomllite::Value::Type::Boolean:
            setting.kind = WiaSetting::Kind::Boolean;
            setting.boolean = value.boolean;
            break;
        default:
        {
            setting.kind = WiaSetting::Kind::String;
            setting.text = strutil::Utf8ToWide(value.str);

            // Строка из одних идентификаторов похожа на константы (FEEDER|DUPLEX): если
            // её не удаётся вычислить, скорее всего это опечатка - предупреждаем в журнале.
            // Ошибкой это не считается: строковое свойство может хранить любой текст.
            long dummy = 0;
            const bool looksLikeConstants = !setting.text.empty() &&
                setting.text.find_first_not_of(L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_| ") == std::wstring::npos &&
                setting.text.find(L'_') != std::wstring::npos;
            if (looksLikeConstants && !wianames::ParseIntegerValue(setting.text, dummy))
            {
                LOG_SAVE << "rus1scan.toml line " << value.line << ": [wia] " << setting.key << " = '" << setting.text
                         << "' - unknown WIA constant, the value will be used as text";
            }
            break;
        }
        }

        settings.push_back(setting);
        return true;
    }

    // Добавляет в текст конфига строку "ключ = значение" со строковым значением.
    //   out   - текст конфига (UTF-8);
    //   key   - имя ключа;
    //   value - значение (UTF-16).
    void AppendStringKey(std::string& out, const char* key, const std::wstring& value)
    {
        out += key;
        out += " = ";
        out += tomllite::QuoteString(strutil::WideToUtf8(value));
        out += "\r\n";
    }
}

// Ищет привязку для пришедшего события WIA. Сначала ищется привязка именно к этому
// устройству, затем - привязка к "любому устройству" (пустой deviceId).
//   deviceId  - идентификатор устройства, от которого пришло событие;
//   eventGuid - GUID события в виде "{...}".
// Возвращает указатель на привязку или nullptr, если событие не настроено.
const EventBinding* Rus1Config::FindForEvent(const std::wstring& deviceId, const std::wstring& eventGuid) const
{
    const EventBinding* anyDevice = nullptr;
    for (const auto& binding : bindings)
    {
        if (!strutil::EqualsNoCase(binding.eventGuid, eventGuid))
        {
            continue;
        }
        if (!binding.deviceId.empty() && strutil::EqualsNoCase(binding.deviceId, deviceId))
        {
            return &binding;
        }
        if (binding.deviceId.empty() && anyDevice == nullptr)
        {
            anyDevice = &binding;
        }
    }
    return anyDevice;
}

// Ищет привязку с точно таким же устройством и событием (без подстановки "любого устройства").
//   deviceId  - идентификатор устройства (пусто = "любое устройство");
//   eventGuid - GUID события.
// Возвращает указатель на привязку или nullptr.
EventBinding* Rus1Config::FindExact(const std::wstring& deviceId, const std::wstring& eventGuid)
{
    for (auto& binding : bindings)
    {
        if (strutil::EqualsNoCase(binding.eventGuid, eventGuid) && strutil::EqualsNoCase(binding.deviceId, deviceId))
        {
            return &binding;
        }
    }
    return nullptr;
}

// Возвращает полный путь к глобальному конфигу: <папка rus1scan.exe>\rus1scan.toml.
// Служба и окно настроек всегда используют один и тот же файл.
std::wstring GetConfigFilePath()
{
    return strutil::GetModuleDir() + L"\\rus1scan.toml";
}

// Преобразует действие в строку для конфига.
//   action - действие.
std::wstring ActionToString(Rus1Action action)
{
    switch (action)
    {
    case Rus1Action::Save:  return L"save";
    case Rus1Action::Print: return L"print";
    case Rus1Action::Start: return L"start";
    default:                return L"nothing";
    }
}

// Разбирает строку действия из конфига (без учёта регистра).
//   text   - "nothing", "save", "print" или "start";
//   action - [out] результат.
// Возвращает false, если строка не распознана.
bool ActionFromString(const std::wstring& text, Rus1Action& action)
{
    const std::wstring t = strutil::Trim(text);
    if (strutil::EqualsNoCase(t, L"nothing")) { action = Rus1Action::Nothing; return true; }
    if (strutil::EqualsNoCase(t, L"save"))    { action = Rus1Action::Save;    return true; }
    if (strutil::EqualsNoCase(t, L"print"))   { action = Rus1Action::Print;   return true; }
    if (strutil::EqualsNoCase(t, L"start"))   { action = Rus1Action::Start;   return true; }
    return false;
}

// Преобразует формат файлов в строку для конфига ("gif" или "bmp").
//   type - формат файлов.
std::wstring FileTypeToString(ImageFileType type)
{
    return type == ImageFileType::Bmp ? L"bmp" : L"gif";
}

// Разбирает значение [common] filetype (без учёта регистра).
//   text - "gif" или "bmp";
//   type - [out] результат.
// Возвращает false, если строка не распознана.
bool FileTypeFromString(const std::wstring& text, ImageFileType& type)
{
    const std::wstring t = strutil::Trim(text);
    if (strutil::EqualsNoCase(t, L"gif")) { type = ImageFileType::Gif; return true; }
    if (strutil::EqualsNoCase(t, L"bmp")) { type = ImageFileType::Bmp; return true; }
    return false;
}

// Читает конфиг из файла TOML.
//   path   - путь к rus1scan.toml;
//   config - [out] настройки; если файла нет, остаются значения по умолчанию;
//   error  - [out] при ошибке: описание с номером строки.
// Возвращает true, если файл прочитан или отсутствует; false - при ошибке чтения/синтаксиса.
bool LoadConfig(const std::wstring& path, Rus1Config& config, std::wstring& error)
{
    config = Rus1Config();
    error.clear();

    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        LOG_SAVE << "Config file not found, using defaults: " << path;
        return true;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        error = L"не удалось открыть файл " + path;
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();

    tomllite::Document doc;
    std::string parseError;
    if (!tomllite::Parse(buffer.str(), doc, parseError))
    {
        error = strutil::Utf8ToWide(parseError);
        return false;
    }

    Rus1Config result;
    for (const auto& table : doc.tables)
    {
        if (table.name.empty())
        {
            for (const auto& entry : table.entries)
            {
                LOG_SAVE << "rus1scan.toml line " << entry.second.line << ": key '" << entry.first
                         << "' outside of any table ignored";
            }
        }
        else if (table.name == "common" && !table.isArrayElement)
        {
            for (const auto& entry : table.entries)
            {
                if (entry.first == "filePrefix")
                {
                    if (!ReadString(entry.second, entry.first, result.filePrefix, error)) return false;
                }
                else if (entry.first == "filetype")
                {
                    std::wstring text;
                    if (!ReadString(entry.second, entry.first, text, error)) return false;
                    if (!FileTypeFromString(text, result.fileType))
                    {
                        error = L"строка " + std::to_wstring(entry.second.line) + L": неизвестный filetype \"" + text +
                                L"\" (допустимо: gif, bmp)";
                        return false;
                    }
                }
                else
                {
                    LOG_SAVE << "rus1scan.toml line " << entry.second.line << ": unknown key '" << entry.first
                             << "' in [common] ignored";
                }
            }
        }
        else if (table.name == "wia" && !table.isArrayElement)
        {
            for (const auto& entry : table.entries)
            {
                if (!ReadWiaSetting(entry.first, entry.second, result.wiaProps, error)) return false;
            }
        }
        else if (table.name == "gui" && !table.isArrayElement)
        {
            for (const auto& entry : table.entries)
            {
                if (entry.first == "showDisconnectedDevices")
                {
                    if (!ReadBool(entry.second, entry.first, result.showDisconnectedDevices, error)) return false;
                }
                else
                {
                    LOG_SAVE << "rus1scan.toml line " << entry.second.line << ": unknown key '" << entry.first
                             << "' in [gui] ignored";
                }
            }
        }
        else if (table.name == "binding" && table.isArrayElement)
        {
            EventBinding binding;
            if (!ReadBinding(table, binding, error)) return false;
            result.bindings.push_back(binding);
        }
        else
        {
            LOG_SAVE << "rus1scan.toml line " << table.line << ": unknown table '" << table.name << "' ignored";
        }
    }

    config = result;
    return true;
}

// Записывает конфиг в файл TOML. Сначала пишется временный файл <path>.tmp, затем он
// атомарно заменяет основной - служба никогда не увидит наполовину записанный конфиг.
// Комментарии, добавленные в файл вручную, при сохранении не сохраняются.
//   path   - путь к rus1scan.toml;
//   config - сохраняемые настройки;
//   error  - [out] описание ошибки (например, нет прав на запись в папку).
// Возвращает true при успехе.
bool SaveConfig(const std::wstring& path, const Rus1Config& config, std::wstring& error)
{
    std::string out;
    out += "# rus1scan.toml - глобальный конфиг rus1scan (кодировка UTF-8).\r\n";
    out += "# Файл перезаписывается окном настроек (двойной клик по rus1scan.exe),\r\n";
    out += "# комментарии, добавленные вручную, при этом не сохраняются.\r\n";
    out += "# Служба rus1scan перечитывает файл автоматически после каждого изменения.\r\n";
    out += "#\r\n";
    out += "# [common]\r\n";
    out += "#   filePrefix - префикс имён файлов сканов (далее дата_время и номер страницы).\r\n";
    out += "#   filetype   - формат файлов при pdf = false: gif (палитра до 256 цветов, малый размер)\r\n";
    out += "#                или bmp (без потерь, большой размер).\r\n";
    out += "#\r\n";
    out += "# [gui]\r\n";
    out += "#   showDisconnectedDevices - показывать в окне настроек отключённые устройства.\r\n";
    out += "#\r\n";
    out += "# [wia] - WIA-свойства, выставляемые перед каждым сканированием: ИМЯ = значение.\r\n";
    out += "#   ИМЯ - любой параметр из WIA_CONSTANTS.TXT.H (WIA_IPS_XRES), его отображаемое имя\r\n";
    out += "#   (\"Horizontal Resolution\") или номер (6147).\r\n";
    out += "#   Значение - число, true/false или строка: константа WIA ('WIA_INTENT_IMAGE_TYPE_COLOR'),\r\n";
    out += "#   несколько констант ('FEEDER|DUPLEX'), дробное число ('8.5'), GUID/формат ('WiaImgFmt_BMP')\r\n";
    out += "#   или текст для строковых свойств. Примеры:\r\n";
    out += "#     WIA_IPS_XRES = 300\r\n";
    out += "#     WIA_IPS_YRES = 300\r\n";
    out += "#     WIA_IPA_DATATYPE = 'WIA_DATA_COLOR'          # WIA_DATA_GRAYSCALE, WIA_DATA_THRESHOLD\r\n";
    out += "#     WIA_DPS_DOCUMENT_HANDLING_SELECT = 'FEEDER'  # 'FLATBED', 'FEEDER|DUPLEX'\r\n";
    out += "#   Если задано разрешение, а область (WIA_IPS_XEXTENT/YEXTENT) нет - сканируется всё стекло.\r\n";
    out += "#   WIA_IPA_FORMAT и WIA_IPA_TYMED задаёт сама программа.\r\n";
    out += "#\r\n";
    out += "# [[binding]] - привязка кнопки/события сканера к действию (по одной на событие):\r\n";
    out += "#   deviceId   - WIA ID устройства; '' = любое устройство;\r\n";
    out += "#   eventGuid  - GUID события WIA; eventFlags: 1 = уведомление, 2 = действие, 3 = оба;\r\n";
    out += "#   action     - nothing | save | print | start;\r\n";
    out += "#   folder     - шаблон папки для save/start: %d %m %Y %y %H %M %S (дата/время);\r\n";
    out += "#   pdf        - true: сохранять все страницы в один PDF, false: файлы формата filetype (по одному на страницу);\r\n";
    out += "#   printer    - принтер для print ('' = принтер по умолчанию);\r\n";
    out += "#   program    - программа для start; %1 заменяется путём к файлу,\r\n";
    out += "#                без %1 путь добавляется в конец командной строки.\r\n";
    out += "\r\n";

    out += "[common]\r\n";
    AppendStringKey(out, "filePrefix", config.filePrefix);
    AppendStringKey(out, "filetype", FileTypeToString(config.fileType));
    out += "\r\n";

    out += "[gui]\r\n";
    out += std::string("showDisconnectedDevices = ") + (config.showDisconnectedDevices ? "true" : "false") + "\r\n";
    out += "\r\n";

    out += "[wia]\r\n";
    for (const auto& setting : config.wiaProps)
    {
        out += tomllite::QuoteKey(strutil::WideToUtf8(setting.key)) + " = ";
        switch (setting.kind)
        {
        case WiaSetting::Kind::Integer: out += std::to_string(setting.integer); break;
        case WiaSetting::Kind::Boolean: out += setting.boolean ? "true" : "false"; break;
        default:                        out += tomllite::QuoteString(strutil::WideToUtf8(setting.text)); break;
        }
        out += "\r\n";
    }

    for (const auto& b : config.bindings)
    {
        out += "\r\n[[binding]]\r\n";
        AppendStringKey(out, "deviceId", b.deviceId);
        AppendStringKey(out, "deviceName", b.deviceName);
        AppendStringKey(out, "eventGuid", b.eventGuid);
        AppendStringKey(out, "eventName", b.eventName);
        out += "eventFlags = " + std::to_string(b.eventFlags) + "\r\n";
        AppendStringKey(out, "action", ActionToString(b.action));
        AppendStringKey(out, "folder", b.folder);
        out += std::string("pdf = ") + (b.pdf ? "true" : "false") + "\r\n";
        AppendStringKey(out, "printer", b.printer);
        AppendStringKey(out, "program", b.program);
    }

    const std::wstring tempPath = path + L".tmp";
    HANDLE file = CreateFileW(tempPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        const DWORD code = GetLastError();
        error = L"не удалось создать файл " + tempPath + L" (ошибка " + std::to_wstring(code) + L")";
        if (code == ERROR_ACCESS_DENIED)
        {
            error += L". Нет прав на запись в папку программы - запустите rus1scan.exe от имени администратора.";
        }
        return false;
    }

    DWORD written = 0;
    const BOOL writeOk = WriteFile(file, out.data(), static_cast<DWORD>(out.size()), &written, NULL);
    const DWORD writeError = GetLastError();
    FlushFileBuffers(file);
    CloseHandle(file);
    if (!writeOk || written != out.size())
    {
        DeleteFileW(tempPath.c_str());
        error = L"ошибка записи в файл " + tempPath + L" (ошибка " + std::to_wstring(writeError) + L")";
        return false;
    }

    if (!MoveFileExW(tempPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        const DWORD code = GetLastError();
        DeleteFileW(tempPath.c_str());
        error = L"не удалось заменить файл " + path + L" (ошибка " + std::to_wstring(code) + L")";
        return false;
    }

    LOG_SAVE << "Config saved: " << path << " (" << config.bindings.size() << " bindings)";
    return true;
}
