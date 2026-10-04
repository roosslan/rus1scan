// wia_names.h: имена свойств и констант WIA для секции [wia] в rus1scan.toml.
//
// Ключ в [wia] - имя свойства из WIA_CONSTANTS.TXT.H / WiaDef.h (WIA_IPS_XRES),
// его отображаемое имя ("Horizontal Resolution") или номер (6147).
// Значение - число, true/false или строка: имя константы (WIA_INTENT_IMAGE_TYPE_COLOR),
// несколько констант через | (FEEDER|DUPLEX), число в виде текста ("0x10", "8.5"),
// GUID / имя формата (WiaImgFmt_BMP) или просто текст для строковых свойств.
// Таблицы генерирует tools\gen_wia_names.py (файл wia_names_table.inc).

#pragma once

#include <string>

// Свойство WIA: имя макроса, номер и отображаемое имя.
struct WiaPropertyName
{
    const wchar_t* name;
    unsigned long  id;
    const wchar_t* display;
};

// Константа значения WIA: имя макроса и число.
struct WiaValueName
{
    const wchar_t* name;
    long           value;
};

namespace wianames
{
    // Находит номер свойства по имени макроса, отображаемому имени или номеру.
    bool FindPropertyId(const std::wstring& key, unsigned long& id);

    // Возвращает имя макроса свойства по номеру (или сам номер, если свойство неизвестно).
    std::wstring PropertyName(unsigned long id);

    // Вычисляет целое значение: число ("300", "0x10") или константы через | ("FEEDER|DUPLEX").
    bool ParseIntegerValue(const std::wstring& text, long& value);

    // Находит GUID формата: "{...}" или имя (WiaImgFmt_BMP, BMP).
    bool ParseFormatGuid(const std::wstring& text, GUID& guid);
}
