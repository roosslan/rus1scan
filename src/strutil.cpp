// strutil.cpp: вспомогательные функции для строк, кодировок, GUID и путей (см. strutil.h).

#include "stdafx.h"
#include "strutil.h"

namespace strutil
{
    // Перекодирует строку из UTF-8 в UTF-16.
    //   utf8 - исходная строка в UTF-8; некорректные последовательности заменяются на U+FFFD.
    // Возвращает строку UTF-16 (пустую, если исходная пустая).
    std::wstring Utf8ToWide(const std::string& utf8)
    {
        if (utf8.empty())
        {
            return std::wstring();
        }
        const int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), NULL, 0);
        if (len <= 0)
        {
            return std::wstring();
        }
        std::wstring result(static_cast<size_t>(len), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &result[0], len);
        return result;
    }

    // Перекодирует строку из UTF-16 в UTF-8.
    //   wide - исходная строка UTF-16.
    // Возвращает строку UTF-8 (пустую, если исходная пустая).
    std::string WideToUtf8(const std::wstring& wide)
    {
        if (wide.empty())
        {
            return std::string();
        }
        const int len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), NULL, 0, NULL, NULL);
        if (len <= 0)
        {
            return std::string();
        }
        std::string result(static_cast<size_t>(len), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), &result[0], len, NULL, NULL);
        return result;
    }

    // Копирует содержимое BSTR в std::wstring.
    //   bstr - строка COM; NULL допустим и означает пустую строку.
    // Длина берётся из самого BSTR (SysStringLen), поэтому встроенные нули не обрезают строку.
    std::wstring BstrToWide(BSTR bstr)
    {
        if (bstr == NULL)
        {
            return std::wstring();
        }
        return std::wstring(bstr, SysStringLen(bstr));
    }

    // Преобразует GUID в текст вида "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}" (заглавные буквы).
    //   guid - преобразуемый GUID.
    std::wstring GuidToString(const GUID& guid)
    {
        wchar_t buf[64] = {};
        if (StringFromGUID2(guid, buf, static_cast<int>(COUNTOF(buf))) == 0)
        {
            return std::wstring();
        }
        return buf;
    }

    // Разбирает текстовый GUID.
    //   text - строка GUID; фигурные скобки и пробелы по краям необязательны;
    //   guid - [out] результат разбора.
    // Возвращает true, если строка является корректным GUID.
    bool StringToGuid(const std::wstring& text, GUID& guid)
    {
        std::wstring t = Trim(text);
        if (t.empty())
        {
            return false;
        }
        if (t.front() != L'{')
        {
            t = L"{" + t + L"}";
        }
        if (t.size() != 38)
        {
            return false;
        }
        return SUCCEEDED(IIDFromString(t.c_str(), &guid));
    }

    // Убирает пробелы, табуляции и переводы строк в начале и в конце строки.
    //   text - исходная строка.
    std::wstring Trim(const std::wstring& text)
    {
        const wchar_t* spaces = L" \t\r\n";
        const size_t first = text.find_first_not_of(spaces);
        if (first == std::wstring::npos)
        {
            return std::wstring();
        }
        const size_t last = text.find_last_not_of(spaces);
        return text.substr(first, last - first + 1);
    }

    // Сравнивает две строки без учёта регистра (порядковое сравнение, не зависит от локали).
    //   a, b - сравниваемые строки.
    // Возвращает true, если строки равны.
    bool EqualsNoCase(const std::wstring& a, const std::wstring& b)
    {
        return CompareStringOrdinal(a.c_str(), static_cast<int>(a.size()),
                                    b.c_str(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
    }

    // Возвращает полный путь к исполняемому файлу текущего процесса.
    // Поддерживает пути длиннее MAX_PATH (буфер увеличивается при необходимости).
    std::wstring GetModulePath()
    {
        std::wstring path(MAX_PATH, L'\0');
        for (;;)
        {
            const DWORD len = GetModuleFileNameW(NULL, &path[0], static_cast<DWORD>(path.size()));
            if (len == 0)
            {
                return std::wstring();
            }
            if (len < path.size())
            {
                path.resize(len);
                return path;
            }
            path.resize(path.size() * 2);
        }
    }

    // Возвращает папку исполняемого файла текущего процесса без завершающей "\".
    std::wstring GetModuleDir()
    {
        const std::wstring path = GetModulePath();
        const size_t slash = path.find_last_of(L"\\/");
        return (slash == std::wstring::npos) ? std::wstring(L".") : path.substr(0, slash);
    }

    // Преобразует HRESULT в строку "0x80004005".
    //   hr - код результата.
    std::wstring HrToString(HRESULT hr)
    {
        wchar_t buf[16];
        swprintf_s(buf, L"0x%08lX", static_cast<unsigned long>(hr));
        return buf;
    }

    // Раскрывает в шаблоне пути коды даты и времени. Поддерживаются:
    //   %d - день (01-31), %m - месяц (01-12), %Y - год (2026), %y - год (26),
    //   %H - часы (00-23), %M - минуты, %S - секунды, %% - знак процента.
    // Прочие последовательности "%x" остаются как есть (в отличие от wcsftime
    // это не приводит к аварийному завершению при опечатке в шаблоне).
    //   pattern - шаблон, например L"C:\\Scans\\%d.%m.%Y\\";
    //   time    - дата и время для подстановки.
    std::wstring ExpandDatePattern(const std::wstring& pattern, const SYSTEMTIME& time)
    {
        std::wstring result;
        result.reserve(pattern.size() + 16);

        for (size_t i = 0; i < pattern.size(); ++i)
        {
            const wchar_t c = pattern[i];
            if (c != L'%' || i + 1 >= pattern.size())
            {
                result += c;
                continue;
            }

            wchar_t buf[8] = {};
            switch (pattern[i + 1])
            {
            case L'd': swprintf_s(buf, L"%02u", time.wDay);          break;
            case L'm': swprintf_s(buf, L"%02u", time.wMonth);        break;
            case L'Y': swprintf_s(buf, L"%04u", time.wYear);         break;
            case L'y': swprintf_s(buf, L"%02u", time.wYear % 100);   break;
            case L'H': swprintf_s(buf, L"%02u", time.wHour);         break;
            case L'M': swprintf_s(buf, L"%02u", time.wMinute);       break;
            case L'S': swprintf_s(buf, L"%02u", time.wSecond);       break;
            case L'%': buf[0] = L'%';                                break;
            default:
                result += c;        // Неизвестный код - оставляем "%" как есть
                continue;
            }
            result += buf;
            ++i;
        }
        return result;
    }

    // Заменяет символы, недопустимые в имени файла Windows (\ / : * ? " < > | и управляющие),
    // на подчёркивание.
    //   name - исходное имя файла (без пути).
    std::wstring SanitizeFileName(const std::wstring& name)
    {
        std::wstring result = name;
        for (wchar_t& c : result)
        {
            if (c < 32 || wcschr(L"\\/:*?\"<>|", c) != nullptr)
            {
                c = L'_';
            }
        }
        return result;
    }
}
