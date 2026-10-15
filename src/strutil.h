// strutil.h: вспомогательные функции для строк, кодировок, GUID и путей.
// Внутри программы весь текст хранится в std::wstring (UTF-16);
// в файлах (журнал, rus1scan.toml) - в UTF-8.

#pragma once

#include <string>

namespace strutil
{
    // UTF-8 -> UTF-16.
    std::wstring Utf8ToWide(const std::string& utf8);

    // UTF-16 -> UTF-8.
    std::string WideToUtf8(const std::wstring& wide);

    // BSTR -> std::wstring (NULL превращается в пустую строку).
    std::wstring BstrToWide(BSTR bstr);

    // GUID -> "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}".
    std::wstring GuidToString(const GUID& guid);

    // Строка (с фигурными скобками или без) -> GUID. Возвращает false при ошибке формата.
    bool StringToGuid(const std::wstring& text, GUID& guid);

    // Убирает пробелы и табуляции по краям.
    std::wstring Trim(const std::wstring& text);

    // Сравнение строк без учёта регистра.
    bool EqualsNoCase(const std::wstring& a, const std::wstring& b);

    // Полный путь к текущему exe.
    std::wstring GetModulePath();

    // Папка текущего exe (без завершающей "\").
    std::wstring GetModuleDir();

    // HRESULT -> "0x80004005".
    std::wstring HrToString(HRESULT hr);

    // Раскрывает в шаблоне пути коды даты/времени (%d %m %Y %y %H %M %S %%).
    std::wstring ExpandDatePattern(const std::wstring& pattern, const SYSTEMTIME& time);

    // Заменяет символы, недопустимые в имени файла, на "_".
    std::wstring SanitizeFileName(const std::wstring& name);
}
