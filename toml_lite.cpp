// toml_lite.cpp: минимальный разборщик TOML для rus1scan.toml (см. toml_lite.h).

#include "stdafx.h"
#include "toml_lite.h"

namespace tomllite
{
    namespace
    {
        // Проверяет, может ли символ входить в простой (не взятый в кавычки) ключ TOML.
        //   c - проверяемый символ.
        bool IsBareKeyChar(char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == '-';
        }

        // Дописывает в строку символ Юникода в кодировке UTF-8.
        //   out - строка-приёмник;
        //   cp  - код символа (0..0x10FFFF, кроме суррогатов).
        void AppendUtf8(std::string& out, unsigned long cp)
        {
            if (cp < 0x80)
            {
                out += static_cast<char>(cp);
            }
            else if (cp < 0x800)
            {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else if (cp < 0x10000)
            {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else
            {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }

        // Разбор одной строки файла TOML. Хранит текущую позицию и номер строки
        // для сообщений об ошибках.
        class LineParser
        {
        public:
            // Создаёт разборщик строки.
            //   text - содержимое строки (без перевода строки);
            //   line - номер строки в файле (с 1).
            LineParser(const std::string& text, int line) : m_s(text), m_pos(0), m_line(line) {}

            // Пропускает пробелы и табуляции с текущей позиции.
            void SkipSpaces()
            {
                while (m_pos < m_s.size() && (m_s[m_pos] == ' ' || m_s[m_pos] == '\t'))
                {
                    ++m_pos;
                }
            }

            // Возвращает true, если после пропуска пробелов строка закончилась или начался комментарий.
            bool AtCommentOrEnd()
            {
                SkipSpaces();
                return m_pos >= m_s.size() || m_s[m_pos] == '#';
            }

            // Возвращает true, если текущая позиция начинается с указанного текста.
            //   text - ожидаемый префикс.
            bool LookingAt(const char* text) const
            {
                return m_s.compare(m_pos, strlen(text), text) == 0;
            }

            // Сдвигает позицию на n символов.
            //   n - на сколько символов сдвинуться.
            void Advance(size_t n) { m_pos += n; }

            // Формирует текст ошибки с номером строки и сохраняет его.
            //   error   - [out] куда записать текст;
            //   message - описание ошибки.
            // Всегда возвращает false (для краткого "return Fail(...)").
            bool Fail(std::string& error, const std::string& message) const
            {
                error = "строка " + std::to_string(m_line) + ": " + message;
                return false;
            }

            // Разбирает ключ: простой (A-Z a-z 0-9 _ -) или в кавычках.
            //   key   - [out] ключ в UTF-8;
            //   error - [out] текст ошибки.
            bool ParseKey(std::string& key, std::string& error)
            {
                SkipSpaces();
                if (m_pos >= m_s.size())
                {
                    return Fail(error, "ожидается ключ");
                }

                const char c = m_s[m_pos];
                if (c == '"')
                {
                    if (!ParseBasicString(key, error))
                    {
                        return false;
                    }
                }
                else if (c == '\'')
                {
                    if (!ParseLiteralString(key, error))
                    {
                        return false;
                    }
                }
                else
                {
                    const size_t start = m_pos;
                    while (m_pos < m_s.size() && IsBareKeyChar(m_s[m_pos]))
                    {
                        ++m_pos;
                    }
                    if (m_pos == start)
                    {
                        return Fail(error, "недопустимый символ в ключе");
                    }
                    key = m_s.substr(start, m_pos - start);
                }

                SkipSpaces();
                if (m_pos < m_s.size() && m_s[m_pos] == '.')
                {
                    return Fail(error, "составные ключи через точку не поддерживаются");
                }
                return true;
            }

            // Разбирает значение: строку, целое число или true/false.
            //   value - [out] значение;
            //   error - [out] текст ошибки.
            bool ParseValue(Value& value, std::string& error)
            {
                SkipSpaces();
                value.line = m_line;
                if (m_pos >= m_s.size())
                {
                    return Fail(error, "ожидается значение");
                }

                const char c = m_s[m_pos];
                if (c == '"')
                {
                    if (LookingAt("\"\"\""))
                    {
                        return Fail(error, "многострочные строки не поддерживаются");
                    }
                    value.type = Value::Type::String;
                    return ParseBasicString(value.str, error);
                }
                if (c == '\'')
                {
                    if (LookingAt("'''"))
                    {
                        return Fail(error, "многострочные строки не поддерживаются");
                    }
                    value.type = Value::Type::String;
                    return ParseLiteralString(value.str, error);
                }
                if (LookingAt("true") && !IsBareKeyChar(CharAt(m_pos + 4)))
                {
                    value.type = Value::Type::Boolean;
                    value.boolean = true;
                    m_pos += 4;
                    return true;
                }
                if (LookingAt("false") && !IsBareKeyChar(CharAt(m_pos + 5)))
                {
                    value.type = Value::Type::Boolean;
                    value.boolean = false;
                    m_pos += 5;
                    return true;
                }
                if (c == '+' || c == '-' || (c >= '0' && c <= '9'))
                {
                    value.type = Value::Type::Integer;
                    return ParseInteger(value.integer, error);
                }
                if (c == '[' || c == '{')
                {
                    return Fail(error, "массивы и inline-таблицы не поддерживаются");
                }
                return Fail(error, "неподдерживаемое значение (ожидается строка в кавычках, целое число, true или false)");
            }

        private:
            // Возвращает символ в позиции pos или '\0' за концом строки.
            //   pos - позиция.
            char CharAt(size_t pos) const
            {
                return pos < m_s.size() ? m_s[pos] : '\0';
            }

            // Разбирает целое число: необязательный знак, цифры, "_" только между цифрами.
            //   result - [out] число;
            //   error  - [out] текст ошибки.
            bool ParseInteger(long long& result, std::string& error)
            {
                bool negative = false;
                if (m_s[m_pos] == '+' || m_s[m_pos] == '-')
                {
                    negative = (m_s[m_pos] == '-');
                    ++m_pos;
                }

                unsigned long long magnitude = 0;
                bool anyDigit = false;
                bool lastWasDigit = false;
                while (m_pos < m_s.size())
                {
                    const char c = m_s[m_pos];
                    if (c >= '0' && c <= '9')
                    {
                        magnitude = magnitude * 10 + static_cast<unsigned long long>(c - '0');
                        if (magnitude > 9223372036854775808ULL)
                        {
                            return Fail(error, "число слишком большое");
                        }
                        anyDigit = true;
                        lastWasDigit = true;
                    }
                    else if (c == '_' && lastWasDigit && CharAt(m_pos + 1) >= '0' && CharAt(m_pos + 1) <= '9')
                    {
                        lastWasDigit = false;
                    }
                    else if (c == '.' || c == 'e' || c == 'E')
                    {
                        return Fail(error, "дробные числа не поддерживаются");
                    }
                    else if (c == 'x' || c == 'o' || c == 'b')
                    {
                        return Fail(error, "шестнадцатеричные/восьмеричные/двоичные числа не поддерживаются");
                    }
                    else
                    {
                        break;
                    }
                    ++m_pos;
                }

                if (!anyDigit)
                {
                    return Fail(error, "ожидается число");
                }
                if (!negative && magnitude > 9223372036854775807ULL)
                {
                    return Fail(error, "число слишком большое");
                }
                result = negative ? static_cast<long long>(0 - magnitude) : static_cast<long long>(magnitude);
                return true;
            }

            // Разбирает обычную строку "..." с escape-последовательностями
            // \" \\ \b \t \n \f \r \uXXXX \UXXXXXXXX.
            //   out   - [out] строка в UTF-8;
            //   error - [out] текст ошибки.
            bool ParseBasicString(std::string& out, std::string& error)
            {
                out.clear();
                ++m_pos;    // открывающая кавычка
                while (m_pos < m_s.size())
                {
                    const char c = m_s[m_pos++];
                    if (c == '"')
                    {
                        return true;
                    }
                    if (c != '\\')
                    {
                        out += c;
                        continue;
                    }
                    if (m_pos >= m_s.size())
                    {
                        break;
                    }
                    const char e = m_s[m_pos++];
                    switch (e)
                    {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case 'b':  out += '\b'; break;
                    case 't':  out += '\t'; break;
                    case 'n':  out += '\n'; break;
                    case 'f':  out += '\f'; break;
                    case 'r':  out += '\r'; break;
                    case 'u':
                    case 'U':
                    {
                        const size_t digits = (e == 'u') ? 4 : 8;
                        if (m_pos + digits > m_s.size())
                        {
                            return Fail(error, "неполная escape-последовательность \\" + std::string(1, e));
                        }
                        unsigned long cp = 0;
                        for (size_t i = 0; i < digits; ++i)
                        {
                            const char h = m_s[m_pos + i];
                            cp <<= 4;
                            if (h >= '0' && h <= '9')      cp |= static_cast<unsigned long>(h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned long>(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned long>(h - 'A' + 10);
                            else return Fail(error, "некорректная escape-последовательность \\" + std::string(1, e));
                        }
                        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
                        {
                            return Fail(error, "недопустимый код символа в \\" + std::string(1, e));
                        }
                        AppendUtf8(out, cp);
                        m_pos += digits;
                        break;
                    }
                    default:
                        return Fail(error, std::string("неизвестная escape-последовательность \\") + e +
                                           " (для путей Windows используйте '...' или удваивайте \\\\)");
                    }
                }
                return Fail(error, "не закрыта кавычка \"");
            }

            // Разбирает литеральную строку '...' (без escape-последовательностей).
            //   out   - [out] строка в UTF-8;
            //   error - [out] текст ошибки.
            bool ParseLiteralString(std::string& out, std::string& error)
            {
                ++m_pos;    // открывающая кавычка
                const size_t end = m_s.find('\'', m_pos);
                if (end == std::string::npos)
                {
                    return Fail(error, "не закрыта кавычка '");
                }
                out = m_s.substr(m_pos, end - m_pos);
                m_pos = end + 1;
                return true;
            }

            const std::string& m_s;
            size_t             m_pos;
            int                m_line;
        };
    }

    // Ищет значение по ключу в таблице.
    //   key - ключ в UTF-8.
    // Возвращает указатель на значение или nullptr.
    const Value* Table::Find(const std::string& key) const
    {
        for (const auto& entry : entries)
        {
            if (entry.first == key)
            {
                return &entry.second;
            }
        }
        return nullptr;
    }

    // Возвращает обычную (не массив) таблицу с указанным именем или nullptr.
    //   name - имя таблицы; пустое имя - корневая таблица.
    const Table* Document::FindTable(const std::string& name) const
    {
        for (const auto& table : tables)
        {
            if (!table.isArrayElement && table.name == name)
            {
                return &table;
            }
        }
        return nullptr;
    }

    // Возвращает все элементы массива таблиц [[name]] в порядке появления в файле.
    //   name - имя массива таблиц.
    std::vector<const Table*> Document::FindArray(const std::string& name) const
    {
        std::vector<const Table*> result;
        for (const auto& table : tables)
        {
            if (table.isArrayElement && table.name == name)
            {
                result.push_back(&table);
            }
        }
        return result;
    }

    // Разбирает текст TOML (поддерживаемое подмножество описано в toml_lite.h).
    //   utf8Text - содержимое файла в UTF-8 (BOM допускается);
    //   doc      - [out] разобранный документ;
    //   error    - [out] при ошибке: "строка N: описание".
    // Возвращает true при успешном разборе.
    bool Parse(const std::string& utf8Text, Document& doc, std::string& error)
    {
        doc.tables.clear();
        doc.tables.push_back(Table());      // корневая таблица

        size_t pos = 0;
        if (utf8Text.compare(0, 3, "\xEF\xBB\xBF") == 0)
        {
            pos = 3;
        }

        int lineNo = 0;
        while (pos <= utf8Text.size())
        {
            size_t eol = utf8Text.find('\n', pos);
            if (eol == std::string::npos)
            {
                eol = utf8Text.size();
            }
            std::string text = utf8Text.substr(pos, eol - pos);
            pos = eol + 1;
            ++lineNo;
            if (!text.empty() && text.back() == '\r')
            {
                text.pop_back();
            }

            LineParser p(text, lineNo);
            if (p.AtCommentOrEnd())
            {
                continue;
            }

            // Заголовок таблицы [name] или [[name]]
            if (p.LookingAt("["))
            {
                const bool isArray = p.LookingAt("[[");
                p.Advance(isArray ? 2 : 1);

                std::string name;
                if (!p.ParseKey(name, error))
                {
                    return false;
                }
                p.SkipSpaces();
                if (!p.LookingAt(isArray ? "]]" : "]"))
                {
                    return p.Fail(error, isArray ? "ожидается ]]" : "ожидается ]");
                }
                p.Advance(isArray ? 2 : 1);
                if (!p.AtCommentOrEnd())
                {
                    return p.Fail(error, "лишние символы после заголовка таблицы");
                }

                for (const auto& existing : doc.tables)
                {
                    if (!existing.name.empty() && existing.name == name &&
                        (!isArray || !existing.isArrayElement))
                    {
                        return p.Fail(error, "таблица [" + name + "] уже объявлена в строке " +
                                             std::to_string(existing.line));
                    }
                }

                Table table;
                table.name = name;
                table.isArrayElement = isArray;
                table.line = lineNo;
                doc.tables.push_back(table);
                continue;
            }

            // Пара ключ = значение
            std::string key;
            if (!p.ParseKey(key, error))
            {
                return false;
            }
            p.SkipSpaces();
            if (!p.LookingAt("="))
            {
                return p.Fail(error, "ожидается '=' после ключа \"" + key + "\"");
            }
            p.Advance(1);

            Value value;
            if (!p.ParseValue(value, error))
            {
                return false;
            }
            if (!p.AtCommentOrEnd())
            {
                return p.Fail(error, "лишние символы после значения ключа \"" + key + "\"");
            }

            Table& current = doc.tables.back();
            if (current.Find(key) != nullptr)
            {
                return p.Fail(error, "ключ \"" + key + "\" повторяется");
            }
            current.entries.emplace_back(key, value);
        }
        return true;
    }

    // Оформляет строку как значение TOML. Если строка не содержит апострофов и
    // управляющих символов, используется литеральная форма '...' (удобно для путей
    // Windows: обратные слэши не нужно удваивать), иначе - обычная "..." с экранированием.
    //   utf8 - строка в UTF-8.
    std::string QuoteString(const std::string& utf8)
    {
        bool literalOk = true;
        for (unsigned char c : utf8)
        {
            if (c == '\'' || (c < 0x20 && c != '\t') || c == 0x7F)
            {
                literalOk = false;
                break;
            }
        }
        if (literalOk)
        {
            return "'" + utf8 + "'";
        }

        std::string out = "\"";
        for (unsigned char c : utf8)
        {
            switch (c)
            {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\t': out += "\\t";  break;
            case '\n': out += "\\n";  break;
            case '\f': out += "\\f";  break;
            case '\r': out += "\\r";  break;
            default:
                if (c < 0x20 || c == 0x7F)
                {
                    char buf[8];
                    sprintf_s(buf, "\\u%04X", c);
                    out += buf;
                }
                else
                {
                    out += static_cast<char>(c);
                }
            }
        }
        out += "\"";
        return out;
    }

    // Оформляет ключ TOML: если все символы допустимы для простого ключа - возвращает как есть,
    // иначе берёт ключ в кавычки.
    //   utf8 - ключ в UTF-8.
    std::string QuoteKey(const std::string& utf8)
    {
        bool bare = !utf8.empty();
        for (char c : utf8)
        {
            if (!IsBareKeyChar(c))
            {
                bare = false;
                break;
            }
        }
        return bare ? utf8 : QuoteString(utf8);
    }
}
