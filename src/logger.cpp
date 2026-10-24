// logger.cpp: реализация простого журнала rus1scan (см. logger.h).

#include "stdafx.h"
#include "logger.h"
#include "strutil.h"

namespace
{
    std::mutex   g_mutex;                          // Защищает все поля ниже
    HANDLE       g_file = INVALID_HANDLE_VALUE;    // Открытый файл журнала текущего дня
    WORD         g_fileDay = 0;                    // День месяца, для которого открыт g_file
    std::wstring g_folder;                         // Папка журнала
    std::wstring g_tag = L"app";                   // Метка режима работы
    bool         g_console = false;                // Дублировать ли строки в консоль
    bool         g_initialized = false;

    // Пытается создать папку журнала и открыть в ней файл на запись.
    // Возвращает true, если в папку можно писать.
    //   folder - проверяемая папка.
    bool IsFolderWritable(const std::wstring& folder)
    {
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);

        const std::wstring probe = folder + L"\\rus1scan_write_test.tmp";
        HANDLE h = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
        if (h == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        CloseHandle(h);
        return true;
    }

    // Выбирает папку журнала: сначала <папка exe>\logs, при отсутствии прав - %TEMP%\rus1scan\logs.
    // Вызывается под g_mutex.
    void ChooseFolderLocked()
    {
        const std::wstring primary = strutil::GetModuleDir() + L"\\logs";
        if (IsFolderWritable(primary))
        {
            g_folder = primary;
            return;
        }

        wchar_t temp[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, temp);
        std::wstring fallback = temp;
        if (!fallback.empty() && fallback.back() == L'\\')
        {
            fallback.pop_back();
        }
        fallback += L"\\rus1scan\\logs";
        IsFolderWritable(fallback);
        g_folder = fallback;
    }

    // Открывает (или создаёт) файл журнала для указанного дня, закрыв предыдущий.
    // Вызывается под g_mutex.
    //   now - текущее локальное время; из него берётся дата для имени файла.
    void OpenFileForDayLocked(const SYSTEMTIME& now)
    {
        if (g_file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_file);
            g_file = INVALID_HANDLE_VALUE;
        }

        wchar_t name[64];
        swprintf_s(name, L"\\rus1scan_%04u-%02u-%02u.log", now.wYear, now.wMonth, now.wDay);
        const std::wstring path = g_folder + name;

        // FILE_APPEND_DATA + общий доступ на запись: служба и окно настроек могут писать
        // в один файл одновременно, каждая строка дописывается одним WriteFile.
        g_file = CreateFileW(path.c_str(), FILE_APPEND_DATA,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        g_fileDay = now.wDay;
    }
}

namespace rLogger
{
    // Инициализирует журнал: выбирает папку для файлов и запоминает метку режима.
    //   tag           - метка режима работы, попадает в каждую строку журнала;
    //   alsoToConsole - дублировать ли строки в консоль процесса.
    void InitLogging(const wchar_t* tag, bool alsoToConsole)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_tag = (tag != nullptr) ? tag : L"app";
        g_console = alsoToConsole;
        if (!g_initialized)
        {
            ChooseFolderLocked();
            g_initialized = true;
        }
    }

    // Возвращает папку, в которую сейчас пишутся файлы журнала.
    std::wstring GetLogFolderPath()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_initialized)
        {
            ChooseFolderLocked();
            g_initialized = true;
        }
        return g_folder;
    }

    // Возвращает HRESULT в виде строки "0x80004005".
    //   hr - код результата COM/Win32.
    std::string HrText(long hr)
    {
        char buf[16];
        sprintf_s(buf, "0x%08lX", static_cast<unsigned long>(hr));
        return buf;
    }

    // Записывает одну готовую строку в файл журнала (и в консоль/отладчик).
    // Добавляет в начало дату, время, метку режима, PID и TID.
    //   utf8Text - текст строки в UTF-8 без перевода строки.
    void WriteLine(const std::string& utf8Text)
    {
        SYSTEMTIME now;
        GetLocalTime(&now);

        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_initialized)
        {
            ChooseFolderLocked();
            g_initialized = true;
        }
        if (g_file == INVALID_HANDLE_VALUE || g_fileDay != now.wDay)
        {
            OpenFileForDayLocked(now);
        }

        char prefix[96];
        sprintf_s(prefix, "%04u-%02u-%02u %02u:%02u:%02u.%03u [%s:%lu:%lu] ",
                  now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
                  now.wMilliseconds, strutil::WideToUtf8(g_tag).c_str(),
                  GetCurrentProcessId(), GetCurrentThreadId());

        const std::string line = std::string(prefix) + utf8Text + "\r\n";

        if (g_file != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            WriteFile(g_file, line.data(), static_cast<DWORD>(line.size()), &written, NULL);
        }

        const std::wstring wide = strutil::Utf8ToWide(line);
        OutputDebugStringW(wide.c_str());

        if (g_console)
        {
            HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
            if (out != NULL && out != INVALID_HANDLE_VALUE)
            {
                DWORD written = 0;
                if (!WriteConsoleW(out, wide.c_str(), static_cast<DWORD>(wide.size()), &written, NULL))
                {
                    // Вывод перенаправлен в файл/канал - пишем байты UTF-8 как есть
                    WriteFile(out, line.data(), static_cast<DWORD>(line.size()), &written, NULL);
                }
            }
        }
    }

    // Начинает строку журнала: "<файл:строка> функция | ".
    //   file     - полный путь исходного файла (__FILE__), в журнал попадает только имя;
    //   line     - номер строки (__LINE__);
    //   function - имя функции (__FUNCTION__).
    LogLine::LogLine(const char* file, int line, const char* function)
    {
        const char* name = file;
        for (const char* p = file; *p != '\0'; ++p)
        {
            if (*p == '\\' || *p == '/')
            {
                name = p + 1;
            }
        }
        m_stream << "<" << name << ":" << line << "> " << function << " | ";
    }

    // Записывает накопленную строку в журнал.
    LogLine::~LogLine()
    {
        WriteLine(m_stream.str());
    }

    // Добавляет широкую строку, перекодируя её в UTF-8.
    //   text - строка (может быть BSTR или nullptr).
    LogLine& LogLine::operator<<(const wchar_t* text)
    {
        if (text == nullptr)
        {
            m_stream << "(null)";
        }
        else
        {
            m_stream << strutil::WideToUtf8(text);
        }
        return *this;
    }

    // Добавляет std::wstring, перекодируя её в UTF-8.
    //   text - строка.
    LogLine& LogLine::operator<<(const std::wstring& text)
    {
        m_stream << strutil::WideToUtf8(text);
        return *this;
    }
}
