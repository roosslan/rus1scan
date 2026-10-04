// toml_lite.h: минимальный разборщик TOML для файла rus1scan.toml.
//
// Поддерживается подмножество TOML v1.0, которого достаточно для конфига:
//   - комментарии "# ...";
//   - таблицы [имя] и массивы таблиц [[имя]];
//   - ключи: простые (A-Z a-z 0-9 _ -) и в кавычках ("..." или '...');
//   - значения: строки "..." (с escape-последовательностями) и '...' (литеральные),
//     целые числа (со знаком и "_" между цифрами), true / false.
// Не поддерживаются (дают понятную ошибку с номером строки): многострочные строки,
// дробные числа, даты, массивы, inline-таблицы, составные ключи через точку.

#pragma once

#include <string>
#include <vector>
#include <utility>

namespace tomllite
{
    // Значение ключа.
    struct Value
    {
        enum class Type { String, Integer, Boolean };

        Type        type = Type::String;
        std::string str;            // Для String: текст в UTF-8
        long long   integer = 0;    // Для Integer
        bool        boolean = false;// Для Boolean
        int         line = 0;       // Номер строки в файле (для сообщений об ошибках)
    };

    // Таблица: [name] или один элемент массива [[name]]. Корневая таблица имеет пустое имя.
    struct Table
    {
        std::string name;
        bool        isArrayElement = false;
        int         line = 0;
        std::vector<std::pair<std::string, Value>> entries;

        // Ищет значение по ключу; возвращает nullptr, если ключа нет.
        const Value* Find(const std::string& key) const;
    };

    // Разобранный документ: таблицы в порядке появления в файле (первая - корневая).
    struct Document
    {
        std::vector<Table> tables;

        // Возвращает обычную таблицу [name] или nullptr.
        const Table* FindTable(const std::string& name) const;

        // Возвращает все элементы массива таблиц [[name]] в порядке появления.
        std::vector<const Table*> FindArray(const std::string& name) const;
    };

    // Разбирает текст TOML.
    bool Parse(const std::string& utf8Text, Document& doc, std::string& error);

    // Оформляет строку как значение TOML (литеральная '...' или обычная "..." с экранированием).
    std::string QuoteString(const std::string& utf8);

    // Оформляет ключ TOML: простой ключ как есть, иначе в кавычках.
    std::string QuoteKey(const std::string& utf8);
}
