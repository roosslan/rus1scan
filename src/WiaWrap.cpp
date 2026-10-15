/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Основано на примере Microsoft "WiaWrap" (см. WiaWrap.h).

--*/

#include "stdafx.h"
#include "WiaWrap.h"
#include "strutil.h"

namespace WiaWrap
{

//////////////////////////////////////////////////////////////////////////
//
// ReadPropertyLong
//
// Читает свойство WIA и приводит его к LONG. Поддерживаются все целочисленные
// типы VARIANT, а также VT_R4/VT_R8 (с округлением).
//   pWiaPropertyStorage - хранилище свойств устройства или элемента WIA;
//   pPropSpec           - какое свойство читать (обычно PRSPEC_PROPID + номер);
//   plResult            - [out] значение свойства.
// Возвращает S_OK; E_FAIL, если свойства нет или у него нечисловой тип;
// код ошибки ReadMultiple при сбое чтения.
//

HRESULT ReadPropertyLong(
    IWiaPropertyStorage* pWiaPropertyStorage,
    const PROPSPEC* pPropSpec,
    LONG* plResult
)
{
    if (pWiaPropertyStorage == NULL || pPropSpec == NULL || plResult == NULL)
    {
        return E_POINTER;
    }

    PROPVARIANT PropVariant;
    PropVariantInit(&PropVariant);

    HRESULT hr = pWiaPropertyStorage->ReadMultiple(1, pPropSpec, &PropVariant);

    // Если ReadMultiple вернул S_FALSE, свойства нет и PropVariant имеет тип VT_EMPTY -
    // это обрабатывается веткой default (E_FAIL).

    if (SUCCEEDED(hr))
    {
        switch (PropVariant.vt)
        {
        case VT_I1:   *plResult = (LONG)PropVariant.cVal;               hr = S_OK; break;
        case VT_UI1:  *plResult = (LONG)PropVariant.bVal;               hr = S_OK; break;
        case VT_I2:   *plResult = (LONG)PropVariant.iVal;               hr = S_OK; break;
        case VT_UI2:  *plResult = (LONG)PropVariant.uiVal;              hr = S_OK; break;
        case VT_I4:   *plResult = (LONG)PropVariant.lVal;               hr = S_OK; break;
        case VT_UI4:  *plResult = (LONG)PropVariant.ulVal;              hr = S_OK; break;
        case VT_INT:  *plResult = (LONG)PropVariant.intVal;             hr = S_OK; break;
        case VT_UINT: *plResult = (LONG)PropVariant.uintVal;            hr = S_OK; break;
        case VT_R4:   *plResult = (LONG)(PropVariant.fltVal + 0.5);     hr = S_OK; break;
        case VT_R8:   *plResult = (LONG)(PropVariant.dblVal + 0.5);     hr = S_OK; break;
        default:      hr = E_FAIL; break;
        }
    }

    PropVariantClear(&PropVariant);

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// ReadPropertyGuid
//
// Читает свойство WIA типа GUID. Поддерживаются VT_CLSID, а также GUID в виде
// строки (VT_BSTR, VT_LPWSTR, VT_LPSTR).
//   pWiaPropertyStorage - хранилище свойств;
//   pPropSpec           - какое свойство читать;
//   pguidResult         - [out] значение свойства.
// Возвращает S_OK; E_FAIL, если свойства нет или тип не подходит.
//

HRESULT ReadPropertyGuid(
    IWiaPropertyStorage* pWiaPropertyStorage,
    const PROPSPEC* pPropSpec,
    GUID* pguidResult
)
{
    if (pWiaPropertyStorage == NULL || pPropSpec == NULL || pguidResult == NULL)
    {
        return E_POINTER;
    }

    PROPVARIANT PropVariant;
    PropVariantInit(&PropVariant);

    HRESULT hr = pWiaPropertyStorage->ReadMultiple(1, pPropSpec, &PropVariant);

    if (SUCCEEDED(hr))
    {
        switch (PropVariant.vt)
        {
        case VT_CLSID:
            *pguidResult = *PropVariant.puuid;
            hr = S_OK;
            break;

        case VT_BSTR:
            hr = CLSIDFromString(PropVariant.bstrVal, pguidResult);
            break;

        case VT_LPWSTR:
            hr = CLSIDFromString(PropVariant.pwszVal, pguidResult);
            break;

        case VT_LPSTR:
        {
            WCHAR wszGuid[40] = {};
            mbstowcs_s(NULL, wszGuid, COUNTOF(wszGuid), PropVariant.pszVal, _TRUNCATE);
            hr = CLSIDFromString(wszGuid, pguidResult);
            break;
        }

        default:
            hr = E_FAIL;
            break;
        }
    }

    PropVariantClear(&PropVariant);

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// ReadPropertyString
//
// Читает строковое свойство WIA (например WIA_DIP_DEV_ID, WIA_DIP_DEV_NAME).
//   pWiaPropertyStorage - хранилище свойств;
//   propId              - номер свойства (WIA_DIP_..., WIA_IPA_...);
//   pResult             - [out] значение свойства.
// Возвращает S_OK; E_FAIL, если свойства нет или оно не строковое.
//

HRESULT ReadPropertyString(IWiaPropertyStorage* pWiaPropertyStorage, PROPID propId, std::wstring* pResult)
{
    if (pWiaPropertyStorage == NULL || pResult == NULL)
    {
        return E_POINTER;
    }

    PROPSPEC spec;
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = propId;

    PROPVARIANT PropVariant;
    PropVariantInit(&PropVariant);

    HRESULT hr = pWiaPropertyStorage->ReadMultiple(1, &spec, &PropVariant);

    if (SUCCEEDED(hr))
    {
        if (PropVariant.vt == VT_BSTR)
        {
            *pResult = strutil::BstrToWide(PropVariant.bstrVal);
            hr = S_OK;
        }
        else if (PropVariant.vt == VT_LPWSTR && PropVariant.pwszVal != NULL)
        {
            *pResult = PropVariant.pwszVal;
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    PropVariantClear(&PropVariant);

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// ReadPropertyRangeMax
//
// Читает наибольшее допустимое значение свойства WIA, у которого допустимые значения
// заданы диапазоном (WIA_PROP_RANGE), например область сканирования WIA_IPS_XEXTENT.
// Диапазон зависит от текущих значений других свойств (разрешения), поэтому читать его
// нужно после их установки.
//   pWiaPropertyStorage - хранилище свойств элемента;
//   propId              - номер свойства;
//   plMax               - [out] наибольшее допустимое значение.
// Возвращает S_OK; E_FAIL, если у свойства нет диапазона.
//

HRESULT ReadPropertyRangeMax(IWiaPropertyStorage* pWiaPropertyStorage, PROPID propId, LONG* plMax)
{
    if (pWiaPropertyStorage == NULL || plMax == NULL)
    {
        return E_POINTER;
    }

    PROPSPEC spec;
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = propId;

    ULONG accessFlags = 0;
    PROPVARIANT validValues;
    PropVariantInit(&validValues);

    HRESULT hr = pWiaPropertyStorage->GetPropertyAttributes(1, &spec, &accessFlags, &validValues);
    if (SUCCEEDED(hr))
    {
        if ((accessFlags & WIA_PROP_RANGE) && validValues.vt == (VT_VECTOR | VT_I4) &&
            validValues.cal.cElems > WIA_RANGE_MAX)
        {
            *plMax = validValues.cal.pElems[WIA_RANGE_MAX];
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    PropVariantClear(&validValues);

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// WritePropertyLong
//
// Записывает целочисленное (VT_I4) свойство WIA.
//   pWiaPropertyStorage - хранилище свойств;
//   propId              - номер свойства (например WIA_IPS_XRES);
//   lValue              - новое значение.
// Возвращает результат IWiaPropertyStorage::WriteMultiple.
//

HRESULT WritePropertyLong(IWiaPropertyStorage* pWiaPropertyStorage, PROPID propId, LONG lValue)
{
    if (pWiaPropertyStorage == NULL)
    {
        return E_POINTER;
    }

    PROPSPEC spec;
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = propId;

    PROPVARIANT var;
    PropVariantInit(&var);
    var.vt = VT_I4;
    var.lVal = lValue;

    return pWiaPropertyStorage->WriteMultiple(1, &spec, &var, WIA_IPA_FIRST);
}

//////////////////////////////////////////////////////////////////////////
//
// WritePropertyGuid
//
// Записывает свойство WIA типа GUID (VT_CLSID), например формат передачи WIA_IPA_FORMAT.
//   pWiaPropertyStorage - хранилище свойств;
//   propId              - номер свойства;
//   guidValue           - новое значение.
// Возвращает результат IWiaPropertyStorage::WriteMultiple.
//

HRESULT WritePropertyGuid(IWiaPropertyStorage* pWiaPropertyStorage, PROPID propId, const GUID& guidValue)
{
    if (pWiaPropertyStorage == NULL)
    {
        return E_POINTER;
    }

    PROPSPEC spec;
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = propId;

    GUID value = guidValue;

    PROPVARIANT var;
    PropVariantInit(&var);
    var.vt = VT_CLSID;
    var.puuid = &value;     // Память принадлежит нам - PropVariantClear для var не вызываем

    return pWiaPropertyStorage->WriteMultiple(1, &spec, &var, WIA_IPA_FIRST);
}

//////////////////////////////////////////////////////////////////////////
//
// WiaGetNumDevices
//
// Возвращает количество WIA-устройств, зарегистрированных в системе.
//   pSuppliedWiaDevMgr - менеджер устройств WIA; NULL - создать собственный;
//   pulNumDevices      - [out] количество устройств.
// Возвращает S_OK или код ошибки COM.
//

HRESULT WiaGetNumDevices(
    IWiaDevMgr* pSuppliedWiaDevMgr,
    ULONG* pulNumDevices
)
{
    HRESULT hr;

    if (pulNumDevices == NULL)
    {
        return E_POINTER;
    }

    *pulNumDevices = 0;

    // Подключаемся к локальному менеджеру устройств WIA, если он не передан

    CComPtr<IWiaDevMgr> pWiaDevMgr = pSuppliedWiaDevMgr;

    if (pWiaDevMgr == NULL)
    {
        hr = pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    // Получаем перечислитель устройств и число устройств в нём

    CComPtr<IEnumWIA_DEV_INFO> pIEnumWIA_DEV_INFO;

    hr = pWiaDevMgr->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &pIEnumWIA_DEV_INFO);

    if (FAILED(hr))
    {
        return hr;
    }

    ULONG celt = 0;

    hr = pIEnumWIA_DEV_INFO->GetCount(&celt);

    if (FAILED(hr))
    {
        return hr;
    }

    *pulNumDevices = celt;

    return S_OK;
}

}; // namespace WiaWrap
