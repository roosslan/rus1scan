// rus1scan.cpp: работа с устройствами WIA (см. rus1scan.h).

#include "stdafx.h"
#include "rus1scan.h"
#include "WiaWrap.h"
#include "DataCallback.h"
#include "strutil.h"
#include "wia_names.h"

// Подключается к менеджеру устройств WIA (служба stisvc, внепроцессный COM-сервер).
//   manager - [out] указатель на IWiaDevMgr.
// Возвращает S_OK или код ошибки CoCreateInstance.
HRESULT CRus1scan::CreateDeviceManager(CComPtr<IWiaDevMgr>& manager)
{
    manager.Release();
    const HRESULT hr = manager.CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER);
    if (FAILED(hr))
    {
        LOG_SAVE << "CoCreateInstance(CLSID_WiaDevMgr) failed: " << rLogger::HrText(hr);
    }
    return hr;
}

// Возвращает множество ID подключённых сейчас устройств WIA (в верхнем регистре).
//   manager - менеджер устройств WIA.
static std::set<std::wstring> GetConnectedDeviceIds(IWiaDevMgr* manager)
{
    std::set<std::wstring> ids;
    CComPtr<IEnumWIA_DEV_INFO> enumInfo;
    if (FAILED(manager->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &enumInfo)) || enumInfo == NULL)
    {
        return ids;
    }
    for (;;)
    {
        CComPtr<IWiaPropertyStorage> storage;
        ULONG fetched = 0;
        if (enumInfo->Next(1, &storage, &fetched) != S_OK || storage == NULL)
        {
            break;
        }
        std::wstring id;
        if (SUCCEEDED(WiaWrap::ReadPropertyString(storage, WIA_DIP_DEV_ID, &id)) && !id.empty())
        {
            CharUpperBuffW(&id[0], static_cast<DWORD>(id.size()));
            ids.insert(id);
        }
    }
    return ids;
}

// Перечисляет устройства WIA. Для каждого устройства читаются ID, имя, описание, тип и,
// для подключённых, список событий (кнопок). Подключённое устройство, которое не удалось
// открыть, всё равно попадает в список, но без событий.
//   devices             - [out] список устройств (поле online показывает, подключено ли оно);
//   includeDisconnected - true: также установленные в системе, но сейчас отключённые
//                         устройства (WIA_DEVINFO_ENUM_ALL).
// Возвращает S_OK или код ошибки подключения к WIA.
HRESULT CRus1scan::EnumerateDevices(std::vector<WiaDeviceInfo>& devices, bool includeDisconnected)
{
    devices.clear();

    CComPtr<IWiaDevMgr> manager;
    HRESULT hr = CreateDeviceManager(manager);
    if (FAILED(hr))
    {
        return hr;
    }

    std::set<std::wstring> connectedIds;
    CComPtr<IEnumWIA_DEV_INFO> enumInfo;
    if (includeDisconnected)
    {
        connectedIds = GetConnectedDeviceIds(manager);
        hr = manager->EnumDeviceInfo(WIA_DEVINFO_ENUM_ALL, &enumInfo);
        if (FAILED(hr))
        {
            LOG_SAVE << "EnumDeviceInfo(WIA_DEVINFO_ENUM_ALL) failed: " << rLogger::HrText(hr)
                     << " - listing connected devices only";
            includeDisconnected = false;
            enumInfo.Release();
        }
    }
    if (enumInfo == NULL)
    {
        hr = manager->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &enumInfo);
        if (FAILED(hr))
        {
            LOG_SAVE << "IWiaDevMgr::EnumDeviceInfo failed: " << rLogger::HrText(hr);
            return hr;
        }
    }

    for (;;)
    {
        CComPtr<IWiaPropertyStorage> storage;
        ULONG fetched = 0;
        hr = enumInfo->Next(1, &storage, &fetched);
        if (hr != S_OK || storage == NULL)
        {
            break;      // S_FALSE - устройства закончились
        }

        WiaDeviceInfo info;
        WiaWrap::ReadPropertyString(storage, WIA_DIP_DEV_ID, &info.id);
        WiaWrap::ReadPropertyString(storage, WIA_DIP_DEV_NAME, &info.name);
        WiaWrap::ReadPropertyString(storage, WIA_DIP_DEV_DESC, &info.description);

        PROPSPEC typeSpec;
        typeSpec.ulKind = PRSPEC_PROPID;
        typeSpec.propid = WIA_DIP_DEV_TYPE;
        WiaWrap::ReadPropertyLong(storage, &typeSpec, &info.type);

        if (info.id.empty())
        {
            LOG_SAVE << "WIA device without WIA_DIP_DEV_ID skipped";
            continue;
        }
        if (info.name.empty())
        {
            info.name = info.description.empty() ? info.id : info.description;
        }

        if (includeDisconnected)
        {
            std::wstring upperId = info.id;
            CharUpperBuffW(&upperId[0], static_cast<DWORD>(upperId.size()));
            info.online = connectedIds.count(upperId) != 0;
        }

        if (info.online)
        {
            CComPtr<IWiaItem> root;
            const HRESULT hrDevice = manager->CreateDevice(CComBSTR(info.id.c_str()), &root);
            if (SUCCEEDED(hrDevice) && root != NULL)
            {
                ReadDeviceEvents(root, info.events);
            }
            else
            {
                LOG_SAVE << "CreateDevice(" << info.id << ") failed: " << rLogger::HrText(hrDevice);
            }
        }

        LOG_SAVE << "WIA device: '" << info.name << "' id=" << info.id << " type=" << info.type
                 << (info.online ? " connected" : " disconnected") << " events=" << info.events.size();
        devices.push_back(info);
    }

    return FAILED(hr) ? hr : S_OK;
}

// Читает список событий (кнопок и т.п.) устройства WIA. События подключения и
// отключения устройства пропускаются - к ним нельзя привязать сканирование.
//   rootItem - корневой элемент устройства (результат IWiaDevMgr::CreateDevice);
//   events   - [out] список событий без повторов.
// Возвращает S_OK или код ошибки EnumDeviceCapabilities.
HRESULT CRus1scan::ReadDeviceEvents(IWiaItem* rootItem, std::vector<WiaEventInfo>& events)
{
    events.clear();
    if (rootItem == NULL)
    {
        return E_POINTER;
    }

    CComPtr<IEnumWIA_DEV_CAPS> enumCaps;
    HRESULT hr = rootItem->EnumDeviceCapabilities(WIA_DEVICE_EVENTS, &enumCaps);
    if (FAILED(hr) || enumCaps == NULL)
    {
        LOG_SAVE << "EnumDeviceCapabilities failed: " << rLogger::HrText(hr);
        return FAILED(hr) ? hr : E_FAIL;
    }

    for (;;)
    {
        WIA_DEV_CAP cap = {};
        ULONG fetched = 0;
        hr = enumCaps->Next(1, &cap, &fetched);
        if (hr != S_OK || fetched == 0)
        {
            break;
        }

        WiaEventInfo info;
        info.guid = strutil::GuidToString(cap.guid);
        info.name = strutil::BstrToWide(cap.bstrName);
        info.description = strutil::BstrToWide(cap.bstrDescription);
        info.flags = cap.ulFlags;

        // Строки WIA_DEV_CAP принадлежат вызывающему - освобождаем
        SysFreeString(cap.bstrName);
        SysFreeString(cap.bstrDescription);
        SysFreeString(cap.bstrIcon);
        SysFreeString(cap.bstrCommandline);

        if (IsEqualGUID(cap.guid, WIA_EVENT_DEVICE_CONNECTED) || IsEqualGUID(cap.guid, WIA_EVENT_DEVICE_DISCONNECTED))
        {
            continue;
        }

        bool duplicate = false;
        for (const auto& existing : events)
        {
            duplicate = duplicate || strutil::EqualsNoCase(existing.guid, info.guid);
        }
        if (!duplicate)
        {
            LOG_SAVE << "  event " << info.guid << " '" << info.name << "' flags=" << info.flags;
            events.push_back(info);
        }
    }

    return S_OK;
}

// Сканирует изображение(я) с устройства без пользовательского интерфейса.
// Порядок: открыть устройство -> найти элемент для сканирования -> выставить
// WIA-свойства из конфига -> включить все страницы для автоподатчика -> передать данные.
//   deviceId - WIA ID устройства (из события);
//   itemName - полное имя элемента из события (может быть пустым - тогда берётся
//              первый элемент-изображение устройства);
//   wiaProps - свойства из секции [wia] конфига;
//   images   - [out] полученные страницы (готовые .bmp в памяти).
// Возвращает S_OK, если получена хотя бы одна страница, иначе код ошибки.
HRESULT CRus1scan::AcquireImages(const std::wstring& deviceId, const std::wstring& itemName,
                                 const WiaPropList& wiaProps, std::vector<ScannedImage>& images)
{
    images.clear();
    if (deviceId.empty())
    {
        return E_INVALIDARG;
    }

    CComPtr<IWiaDevMgr> manager;
    HRESULT hr = CreateDeviceManager(manager);
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IWiaItem> root;
    hr = manager->CreateDevice(CComBSTR(deviceId.c_str()), &root);
    if (FAILED(hr) || root == NULL)
    {
        LOG_SAVE << "CreateDevice(" << deviceId << ") failed: " << rLogger::HrText(hr);
        return FAILED(hr) ? hr : E_FAIL;
    }

    CComPtr<IWiaItem> item;
    hr = FindTransferItem(root, itemName, item);
    if (FAILED(hr))
    {
        return hr;
    }

    ApplyWiaProperties(root, item, wiaProps);

    bool pagesGiven = false;
    for (const auto& setting : wiaProps)
    {
        pagesGiven = pagesGiven || setting.propId == WIA_DPS_PAGES;
    }
    if (!pagesGiven)
    {
        EnableFeederAllPages(root);     // После [wia]: там мог быть выбран автоподатчик
    }

    hr = TransferItem(item, images);
    if (SUCCEEDED(hr) && images.empty())
    {
        LOG_SAVE << "Transfer finished without images";
        hr = E_FAIL;
    }
    return hr;
}

// Находит элемент устройства, с которого нужно сканировать.
//   rootItem - корневой элемент устройства;
//   itemName - полное имя элемента из события WIA (может быть пустым);
//   item     - [out] найденный элемент.
// Сначала ищется элемент по имени из события, затем - первый элемент-изображение
// в дереве устройства, затем используется сам корневой элемент, если он изображение.
// Возвращает S_OK или HRESULT_FROM_WIN32(ERROR_NOT_FOUND).
HRESULT CRus1scan::FindTransferItem(IWiaItem* rootItem, const std::wstring& itemName, CComPtr<IWiaItem>& item)
{
    item.Release();

    if (!itemName.empty())
    {
        CComPtr<IWiaItem> found;
        const HRESULT hr = rootItem->FindItemByName(0, CComBSTR(itemName.c_str()), &found);
        if (hr == S_OK && found != NULL)
        {
            LOG_SAVE << "Using item from event: " << itemName;
            item = found;
            return S_OK;
        }
        LOG_SAVE << "Item '" << itemName << "' from event not found (" << rLogger::HrText(hr) << "), searching";
    }

    if (SUCCEEDED(FindFirstImageItem(rootItem, 0, item)) && item != NULL)
    {
        return S_OK;
    }

    LONG rootType = 0;
    if (SUCCEEDED(rootItem->GetItemType(&rootType)) && (rootType & WiaItemTypeImage))
    {
        item = rootItem;
        return S_OK;
    }

    LOG_SAVE << "No image item found on the device";
    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

// Рекурсивно ищет первый дочерний элемент типа "изображение" (у сканера это обычно
// планшет или автоподатчик).
//   parent - элемент, среди потомков которого ищем;
//   depth  - текущая глубина рекурсии (поиск ограничен 3 уровнями);
//   item   - [out] найденный элемент.
// Возвращает S_OK, если элемент найден, иначе S_FALSE или код ошибки.
HRESULT CRus1scan::FindFirstImageItem(IWiaItem* parent, int depth, CComPtr<IWiaItem>& item)
{
    CComPtr<IEnumWiaItem> enumItems;
    HRESULT hr = parent->EnumChildItems(&enumItems);
    if (FAILED(hr) || enumItems == NULL)
    {
        return FAILED(hr) ? hr : S_FALSE;
    }

    for (;;)
    {
        CComPtr<IWiaItem> child;
        ULONG fetched = 0;
        hr = enumItems->Next(1, &child, &fetched);
        if (hr != S_OK || child == NULL)
        {
            break;
        }

        LONG type = 0;
        child->GetItemType(&type);
        if (type & WiaItemTypeImage)
        {
            item = child;
            return S_OK;
        }
        if ((type & WiaItemTypeFolder) && depth < 3)
        {
            if (FindFirstImageItem(child, depth + 1, item) == S_OK)
            {
                return S_OK;
            }
        }
    }
    return S_FALSE;
}

namespace
{
    // Узнаёт тип (VARTYPE) свойства WIA, читая его текущее значение.
    //   storage - хранилище свойств элемента;
    //   propId  - номер свойства;
    //   vt      - [out] тип значения.
    // Возвращает false, если у элемента нет такого свойства.
    bool ReadPropertyType(IWiaPropertyStorage* storage, PROPID propId, VARTYPE& vt)
    {
        if (storage == NULL)
        {
            return false;
        }
        PROPSPEC spec;
        spec.ulKind = PRSPEC_PROPID;
        spec.propid = propId;
        PROPVARIANT var;
        PropVariantInit(&var);
        const HRESULT hr = storage->ReadMultiple(1, &spec, &var);
        vt = var.vt;
        PropVariantClear(&var);
        return hr == S_OK && vt != VT_EMPTY;
    }

    // Возвращает значение параметра [wia] в виде текста (для журнала и строковых свойств).
    //   setting - параметр из конфига.
    std::wstring SettingText(const WiaSetting& setting)
    {
        switch (setting.kind)
        {
        case WiaSetting::Kind::Integer: return std::to_wstring(setting.integer);
        case WiaSetting::Kind::Boolean: return setting.boolean ? L"true" : L"false";
        default:                        return setting.text;
        }
    }

    // Преобразует значение параметра [wia] к типу свойства устройства.
    //   setting - параметр из конфига;
    //   vt      - тип свойства на устройстве (VT_I4, VT_UI4, VT_R4, VT_BSTR, VT_CLSID, ...);
    //   var     - [out] значение для WriteMultiple; вызывающий освобождает его PropVariantClear;
    //   error   - [out] причина, если преобразовать нельзя.
    bool MakeVariant(const WiaSetting& setting, VARTYPE vt, PROPVARIANT& var, std::wstring& error)
    {
        PropVariantInit(&var);
        switch (vt)
        {
        case VT_I1: case VT_UI1: case VT_I2: case VT_UI2: case VT_I4: case VT_UI4:
        case VT_INT: case VT_UINT: case VT_BOOL:
        {
            long value = 0;
            if (setting.kind == WiaSetting::Kind::Integer)
            {
                value = static_cast<long>(setting.integer);
            }
            else if (setting.kind == WiaSetting::Kind::Boolean)
            {
                value = setting.boolean ? 1 : 0;
            }
            else if (!wianames::ParseIntegerValue(setting.text, value))
            {
                error = L"'" + setting.text + L"' is not a number or known WIA constant";
                return false;
            }
            var.vt = vt;
            switch (vt)
            {
            case VT_I1:   var.cVal = static_cast<CHAR>(value);   break;
            case VT_UI1:  var.bVal = static_cast<UCHAR>(value);  break;
            case VT_I2:   var.iVal = static_cast<SHORT>(value);  break;
            case VT_UI2:  var.uiVal = static_cast<USHORT>(value); break;
            case VT_I4:   var.lVal = value;                      break;
            case VT_UI4:  var.ulVal = static_cast<ULONG>(value); break;
            case VT_INT:  var.intVal = value;                    break;
            case VT_UINT: var.uintVal = static_cast<UINT>(value); break;
            default:      var.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE; break;
            }
            return true;
        }

        case VT_R4:
        case VT_R8:
        {
            double value = 0;
            if (setting.kind == WiaSetting::Kind::Integer)
            {
                value = static_cast<double>(setting.integer);
            }
            else if (setting.kind == WiaSetting::Kind::Boolean)
            {
                value = setting.boolean ? 1.0 : 0.0;
            }
            else
            {
                std::wstring text = strutil::Trim(setting.text);
                for (wchar_t& c : text)
                {
                    if (c == L',') c = L'.';    // "8,5" как в старом конфиге
                }
                wchar_t* end = nullptr;
                value = wcstod(text.c_str(), &end);
                long integer = 0;
                if (text.empty() || end == nullptr || *end != L'\0')
                {
                    if (!wianames::ParseIntegerValue(text, integer))
                    {
                        error = L"'" + setting.text + L"' is not a number";
                        return false;
                    }
                    value = integer;
                }
            }
            var.vt = vt;
            if (vt == VT_R4) var.fltVal = static_cast<float>(value); else var.dblVal = value;
            return true;
        }

        case VT_BSTR:
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(SettingText(setting).c_str());
            return var.bstrVal != NULL;

        case VT_CLSID:
        {
            GUID guid;
            if (setting.kind != WiaSetting::Kind::String || !wianames::ParseFormatGuid(setting.text, guid))
            {
                error = L"'" + SettingText(setting) + L"' is not a GUID or WiaImgFmt_ name";
                return false;
            }
            var.vt = VT_CLSID;
            var.puuid = static_cast<CLSID*>(CoTaskMemAlloc(sizeof(CLSID)));  // Освобождается PropVariantClear
            if (var.puuid == NULL)
            {
                var.vt = VT_EMPTY;
                return false;
            }
            *var.puuid = guid;
            return true;
        }

        default:
            error = L"property type " + std::to_wstring(vt) + L" is not supported";
            return false;
        }
    }
}

// Выставляет WIA-свойства из секции [wia] конфига перед сканированием.
// Для каждого параметра определяется, у кого есть это свойство - у элемента сканирования
// (разрешение, цветность, область) или у корневого элемента (свойства устройства,
// например выбор автоподатчика), и какого оно типа; значение из конфига приводится
// к этому типу (число, константы через |, дробное, строка, GUID) и записывается.
// Ошибки только пишутся в журнал - сканирование продолжается с остальными настройками.
//   rootItem - корневой элемент устройства;
//   item     - элемент, с которого будет сканирование;
//   wiaProps - параметры из секции [wia].
void CRus1scan::ApplyWiaProperties(IWiaItem* rootItem, IWiaItem* item, const WiaPropList& wiaProps)
{
    CComQIPtr<IWiaPropertyStorage> itemProps(item);
    CComQIPtr<IWiaPropertyStorage> rootProps(rootItem);

    for (const auto& setting : wiaProps)
    {
        const std::wstring name = wianames::PropertyName(setting.propId);
        const std::wstring label = L"[wia] " + setting.key + L" (" + name + L", " + std::to_wstring(setting.propId) +
                                   L") = '" + SettingText(setting) + L"'";

        if (setting.propId == WIA_IPA_FORMAT || setting.propId == WIA_IPA_TYMED)
        {
            LOG_SAVE << label << ": ignored - set by rus1scan for the transfer";
            continue;
        }

        IWiaPropertyStorage* target = NULL;
        VARTYPE vt = VT_EMPTY;
        const wchar_t* where = L"";
        if (ReadPropertyType(itemProps, setting.propId, vt))
        {
            target = itemProps;
            where = L"item";
        }
        else if (rootProps != itemProps && ReadPropertyType(rootProps, setting.propId, vt))
        {
            target = rootProps;
            where = L"device";
        }
        if (target == NULL)
        {
            LOG_SAVE << label << ": not supported by this device - skipped";
            continue;
        }

        PROPVARIANT var;
        std::wstring error;
        if (!MakeVariant(setting, vt, var, error))
        {
            LOG_SAVE << label << ": " << error << " - skipped";
            continue;
        }

        PROPSPEC spec;
        spec.ulKind = PRSPEC_PROPID;
        spec.propid = setting.propId;
        const HRESULT hr = target->WriteMultiple(1, &spec, &var, WIA_IPA_FIRST);
        PropVariantClear(&var);

        LOG_SAVE << label << " -> " << where << ": "
                 << (SUCCEEDED(hr) ? std::string("ok") : rLogger::HrText(hr) + " (value not accepted by the device)");
    }

    // Область сканирования (WIA_IPS_XEXTENT/YEXTENT) задаётся в пикселях текущего
    // разрешения и после смены разрешения не пересчитывается - при 150 DPI вместо
    // 50 DPI сканировалась бы только треть стекла. Если разрешение менялось, а область
    // в [wia] не задана, выставляем максимальную область от левого верхнего угла.
    bool resolutionChanged = false;
    bool extentGiven = false;
    for (const auto& setting : wiaProps)
    {
        const unsigned long id = setting.propId;
        resolutionChanged = resolutionChanged || id == WIA_IPS_XRES || id == WIA_IPS_YRES;
        extentGiven = extentGiven || id == WIA_IPS_XEXTENT || id == WIA_IPS_YEXTENT ||
                      id == WIA_IPS_XPOS || id == WIA_IPS_YPOS;
    }
    if (resolutionChanged && !extentGiven && itemProps != NULL)
    {
        WiaWrap::WritePropertyLong(itemProps, WIA_IPS_XPOS, 0);
        WiaWrap::WritePropertyLong(itemProps, WIA_IPS_YPOS, 0);

        const PROPID extents[2] = { WIA_IPS_XEXTENT, WIA_IPS_YEXTENT };
        for (PROPID extent : extents)
        {
            LONG maxValue = 0;
            const HRESULT hrRange = WiaWrap::ReadPropertyRangeMax(itemProps, extent, &maxValue);
            const HRESULT hrWrite = SUCCEEDED(hrRange) ? WiaWrap::WritePropertyLong(itemProps, extent, maxValue) : hrRange;
            LOG_SAVE << "Scan area property " << extent << " = " << maxValue << " (maximum): "
                     << (SUCCEEDED(hrWrite) ? std::string("ok") : rLogger::HrText(hrWrite));
        }
    }
}

// Если у сканера выбран автоподатчик (ADF), разрешает сканирование всех листов из
// лотка (WIA_DPS_PAGES = ALL_PAGES), иначе будет отсканирован только один лист.
//   rootItem - корневой элемент устройства.
void CRus1scan::EnableFeederAllPages(IWiaItem* rootItem)
{
    CComQIPtr<IWiaPropertyStorage> rootProps(rootItem);
    if (rootProps == NULL)
    {
        return;
    }

    PROPSPEC spec;
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = WIA_DPS_DOCUMENT_HANDLING_SELECT;

    LONG handling = 0;
    if (SUCCEEDED(WiaWrap::ReadPropertyLong(rootProps, &spec, &handling)) && (handling & FEEDER))
    {
        const HRESULT hr = WiaWrap::WritePropertyLong(rootProps, WIA_DPS_PAGES, ALL_PAGES);
        LOG_SAVE << "Document feeder selected, WIA_DPS_PAGES=ALL_PAGES: " << rLogger::HrText(hr);
    }
}

// Передаёт данные элемента в память (TYMED_CALLBACK, idtGetBandedData).
// Формат передачи - WiaImgFmt_MEMORYBMP (поддерживается всеми сканерами для
// TYMED_CALLBACK), при отказе - WiaImgFmt_BMP. Каждая страница превращается в
// готовый .bmp-файл в памяти.
//   item   - элемент, с которого сканируем;
//   images - [out] полученные страницы.
// Возвращает результат idtGetBandedData (S_OK / S_FALSE - отмена / код ошибки).
HRESULT CRus1scan::TransferItem(IWiaItem* item, std::vector<ScannedImage>& images)
{
    CComQIPtr<IWiaPropertyStorage> props(item);
    CComQIPtr<IWiaDataTransfer> transfer(item);
    if (props == NULL || transfer == NULL)
    {
        LOG_SAVE << "Item does not support IWiaPropertyStorage/IWiaDataTransfer";
        return E_NOINTERFACE;
    }

    HRESULT hr = WiaWrap::WritePropertyLong(props, WIA_IPA_TYMED, TYMED_CALLBACK);
    if (FAILED(hr))
    {
        LOG_SAVE << "Setting WIA_IPA_TYMED=TYMED_CALLBACK failed: " << rLogger::HrText(hr);
        return hr;
    }

    hr = WiaWrap::WritePropertyGuid(props, WIA_IPA_FORMAT, WiaImgFmt_MEMORYBMP);
    if (FAILED(hr))
    {
        LOG_SAVE << "WiaImgFmt_MEMORYBMP rejected (" << rLogger::HrText(hr) << "), trying WiaImgFmt_BMP";
        hr = WiaWrap::WritePropertyGuid(props, WIA_IPA_FORMAT, WiaImgFmt_BMP);
        if (FAILED(hr))
        {
            LOG_SAVE << "Setting WIA_IPA_FORMAT failed: " << rLogger::HrText(hr);
            return hr;
        }
    }

    PROPSPEC bufferSpec;
    bufferSpec.ulKind = PRSPEC_PROPID;
    bufferSpec.propid = WIA_IPA_BUFFER_SIZE;
    LONG bufferSize = 0;
    if (FAILED(WiaWrap::ReadPropertyLong(props, &bufferSpec, &bufferSize)) || bufferSize <= 0)
    {
        bufferSize = 64 * 1024;
    }

    WiaWrap::CComPtrArray<IStream> streams;
    LONG lastLoggedPercent = -100;
    CComPtr<WiaWrap::CDataCallback> callback(
        new WiaWrap::CDataCallback(TransferProgress, &lastLoggedPercent, &streams.Count(), &streams));

    WIA_DATA_TRANSFER_INFO info = {};
    info.ulSize = sizeof(info);
    info.ulBufferSize = static_cast<ULONG>(2 * bufferSize);
    info.bDoubleBuffer = TRUE;

    LOG_SAVE << "idtGetBandedData started, buffer=" << info.ulBufferSize;
    hr = transfer->idtGetBandedData(&info, callback);
    LOG_SAVE << "idtGetBandedData finished: " << rLogger::HrText(hr) << ", pages=" << streams.Count();

    if (FAILED(hr))
    {
        return hr;
    }

    for (LONG i = 0; i < streams.Count(); ++i)
    {
        IStream* stream = streams[i];
        if (stream == NULL)
        {
            continue;
        }

        STATSTG stat = {};
        if (FAILED(stream->Stat(&stat, STATFLAG_NONAME)) || stat.cbSize.QuadPart == 0 ||
            stat.cbSize.QuadPart > 0x7FFFFFFF)
        {
            LOG_SAVE << "Page " << i << ": invalid stream size";
            continue;
        }

        ScannedImage image;
        image.bmp.resize(static_cast<size_t>(stat.cbSize.QuadPart));

        LARGE_INTEGER zero = {};
        stream->Seek(zero, STREAM_SEEK_SET, NULL);
        ULONG read = 0;
        if (FAILED(stream->Read(image.bmp.data(), static_cast<ULONG>(image.bmp.size()), &read)) ||
            read != image.bmp.size())
        {
            LOG_SAVE << "Page " << i << ": stream read failed";
            continue;
        }

        const BITMAPFILEHEADER* fileHeader = reinterpret_cast<const BITMAPFILEHEADER*>(image.bmp.data());
        if (image.bmp.size() < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) ||
            fileHeader->bfType != MAKEWORD('B', 'M'))
        {
            LOG_SAVE << "Page " << i << ": data is not a BMP image";
            continue;
        }

        LOG_SAVE << "Page " << i << ": " << image.bmp.size() << " bytes";
        images.push_back(std::move(image));
    }

    return hr;
}

// Функция хода передачи для CDataCallback: пишет прогресс в журнал с шагом 25%.
//   lStatus          - статус устройства (IT_STATUS_...);
//   lPercentComplete - процент выполнения;
//   pParam           - указатель на LONG с последним записанным в журнал процентом.
// Всегда возвращает S_OK (передача без интерфейса не отменяется).
HRESULT CALLBACK CRus1scan::TransferProgress(LONG lStatus, LONG lPercentComplete, PVOID pParam)
{
    LONG* lastLogged = static_cast<LONG*>(pParam);
    if (lastLogged != NULL && (lPercentComplete >= *lastLogged + 25 || lPercentComplete < *lastLogged))
    {
        *lastLogged = lPercentComplete;
        LOG_SAVE << "Transfer progress " << lPercentComplete << "% (status " << lStatus << ")";
    }
    return S_OK;
}
