// logger.h: простой потокобезопасный журнал rus1scan (заменяет Boost.Log).
//
// Использование:  LOG_SAVE << "текст " << число << L"широкая строка";
// Каждая строка журнала получает дату/время, режим работы, PID, TID и место в коде.
// Файл журнала: <папка exe>\logs\rus1scan_ГГГГ-ММ-ДД.log (UTF-8); если туда нельзя
// писать (например, exe лежит в Program Files, а окно запущено без прав администратора),
// используется %TEMP%\rus1scan\logs.

#pragma once

#include <string>
#include <sstream>
#include <type_traits>

namespace rLogger
{
    // Инициализирует журнал: выбирает папку для файлов и запоминает метку режима.
    //   tag           - короткая метка режима работы ("svc", "gui", "console", "sti", ...),
    //                   попадает в каждую строку журнала;
    //   alsoToConsole - true, если строки нужно дублировать в консоль (режим /console).
    void InitLogging(const wchar_t* tag, bool alsoToConsole = false);

    // Возвращает папку, в которую сейчас пишутся файлы журнала.
    std::wstring GetLogFolderPath();

    // Записывает одну готовую строку (UTF-8) в журнал.
    //   utf8Text - текст строки без перевода строки.
    void WriteLine(const std::string& utf8Text);

    // Возвращает HRESULT в виде строки "0x80004005".
    //   hr - код результата COM/Win32.
    std::string HrText(long hr);

    // Одна строка журнала. Создаётся макросом LOG_SAVE, накапливает текст через <<
    // и записывает его в журнал в деструкторе (то есть в конце выражения).
    class LogLine
    {
    public:
        // Начинает строку журнала с указанием места в исходнике.
        //   file     - имя исходного файла (__FILE__);
        //   line     - номер строки (__LINE__);
        //   function - имя функции (__FUNCTION__).
        LogLine(const char* file, int line, const char* function);

        // Записывает накопленную строку в журнал.
        ~LogLine();

        LogLine(const LogLine&) = delete;
        LogLine& operator=(const LogLine&) = delete;

        // Добавляет в строку любое значение, которое умеет выводить std::ostream
        // (числа, узкие строки UTF-8 и т.п.). Широкие строки обрабатываются перегрузками ниже.
        //   value - выводимое значение.
        template <class T,
                  class = std::enable_if_t<!std::is_convertible_v<const T&, const wchar_t*> &&
                                           !std::is_same_v<std::decay_t<T>, std::wstring>>>
        LogLine& operator<<(const T& value)
        {
            m_stream << value;
            return *this;
        }

        // Добавляет широкую строку (в том числе BSTR и CStringW), перекодируя её в UTF-8.
        //   text - строка; nullptr выводится как "(null)".
        LogLine& operator<<(const wchar_t* text);

        // Добавляет std::wstring, перекодируя её в UTF-8.
        //   text - строка.
        LogLine& operator<<(const std::wstring& text);

    private:
        std::ostringstream m_stream;
    };
}

#define LOG_SAVE ::rLogger::LogLine(__FILE__, __LINE__, __FUNCTION__)
