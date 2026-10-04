// wia_names.cpp: имена свойств и констант WIA (см. wia_names.h).

#include "stdafx.h"
#include "wia_names.h"
#include "strutil.h"

#include "wia_names_table.inc"

namespace
{
    // Форматы изображений WIA, которые можно указать по имени (для VT_CLSID-свойств,
    // например WIA_IPA_PREFERRED_FORMAT).
    struct FormatName
    {
        const wchar_t* name;
        const GUID*    guid;
    };

    const FormatName kFormats[] =
    {
        { L"WiaImgFmt_MEMORYBMP", &WiaImgFmt_MEMORYBMP },
        { L"WiaImgFmt_BMP",       &WiaImgFmt_BMP },
        { L"WiaImgFmt_EMF",       &WiaImgFmt_EMF },
        { L"WiaImgFmt_WMF",       &WiaImgFmt_WMF },
        { L"WiaImgFmt_JPEG",      &WiaImgFmt_JPEG },
        { L"WiaImgFmt_PNG",       &WiaImgFmt_PNG },
        { L"WiaImgFmt_GIF",       &WiaImgFmt_GIF },
        { L"WiaImgFmt_TIFF",      &WiaImgFmt_TIFF },
        { L"WiaImgFmt_EXIF",      &WiaImgFmt_EXIF },
        { L"WiaImgFmt_PHOTOCD",   &WiaImgFmt_PHOTOCD },
        { L"WiaImgFmt_FLASHPIX",  &WiaImgFmt_FLASHPIX },
        { L"WiaImgFmt_ICO",       &WiaImgFmt_ICO },
        { L"WiaImgFmt_RAWRGB",    &WiaImgFmt_RAWRGB },
        { L"WiaImgFmt_JPEG2K",    &WiaImgFmt_JPEG2K },
        { L"WiaImgFmt_XPS",       &WiaImgFmt_XPS },
        { L"WiaImgFmt_PDFA",      &WiaImgFmt_PDFA },
    };

    // Разбирает одно число: десятичное ("300", "-5") или шестнадцатеричное ("0x10").
    //   text  - текст без пробелов по краям;
    //   value - [out] результат.
    // Возвращает false, если текст не является числом в пределах LONG/ULONG.
    bool ParseNumber(const std::wstring& text, long& value)
    {
        if (text.empty())
        {
            return false;
        }
        wchar_t* end = nullptr;
        errno = 0;
        const long long parsed = wcstoll(text.c_str(), &end, 0);
        if (end == nullptr || *end != L'\0' || errno == ERANGE || parsed < LONG_MIN || parsed > 0xFFFFFFFFLL)
        {
            return false;
        }
        value = static_cast<long>(parsed);
        return true;
    }
}

namespace wianames
{
    // Находит номер свойства WIA.
    //   key - имя макроса (WIA_IPS_XRES, без учёта регистра), отображаемое имя
    //         ("Horizontal Resolution") или номер ("6147");
    //   id  - [out] номер свойства.
    // Возвращает false, если свойство не найдено.
    bool FindPropertyId(const std::wstring& key, unsigned long& id)
    {
        const std::wstring k = strutil::Trim(key);
        for (const auto& prop : kWiaPropertyNames)
        {
            if (strutil::EqualsNoCase(k, prop.name) || strutil::EqualsNoCase(k, prop.display))
            {
                id = prop.id;
                return true;
            }
        }

        long number = 0;
        if (ParseNumber(k, number) && number > 0)
        {
            id = static_cast<unsigned long>(number);
            return true;
        }
        return false;
    }

    // Возвращает имя макроса свойства (для журнала); неизвестное свойство - его номер.
    //   id - номер свойства.
    std::wstring PropertyName(unsigned long id)
    {
        for (const auto& prop : kWiaPropertyNames)
        {
            if (prop.id == id)
            {
                return prop.name;
            }
        }
        return std::to_wstring(id);
    }

    // Вычисляет целое значение для числового свойства WIA.
    //   text  - число ("300", "0x10") или одна/несколько констант через "|"
    //           ("WIA_INTENT_IMAGE_TYPE_COLOR", "FEEDER|DUPLEX"); числа и константы
    //           можно смешивать, части объединяются побитовым ИЛИ;
    //   value - [out] результат.
    // Возвращает false, если какая-то часть не является числом или известной константой.
    bool ParseIntegerValue(const std::wstring& text, long& value)
    {
        long result = 0;
        size_t start = 0;
        bool any = false;
        while (start <= text.size())
        {
            size_t bar = text.find(L'|', start);
            if (bar == std::wstring::npos)
            {
                bar = text.size();
            }
            const std::wstring part = strutil::Trim(text.substr(start, bar - start));
            start = bar + 1;

            long partValue = 0;
            bool found = ParseNumber(part, partValue);
            for (size_t i = 0; !found && i < COUNTOF(kWiaValueNames); ++i)
            {
                if (strutil::EqualsNoCase(part, kWiaValueNames[i].name))
                {
                    partValue = kWiaValueNames[i].value;
                    found = true;
                }
            }
            if (!found)
            {
                return false;
            }
            result |= partValue;
            any = true;
        }
        if (!any)
        {
            return false;
        }
        value = result;
        return true;
    }

    // Находит GUID формата изображения.
    //   text - GUID в виде "{...}" или имя формата (WiaImgFmt_BMP; префикс WiaImgFmt_ можно опустить);
    //   guid - [out] результат.
    // Возвращает false, если формат не распознан.
    bool ParseFormatGuid(const std::wstring& text, GUID& guid)
    {
        const std::wstring t = strutil::Trim(text);
        if (strutil::StringToGuid(t, guid))
        {
            return true;
        }
        for (const auto& format : kFormats)
        {
            if (strutil::EqualsNoCase(t, format.name) || strutil::EqualsNoCase(L"WiaImgFmt_" + t, format.name))
            {
                guid = *format.guid;
                return true;
            }
        }
        return false;
    }
}
