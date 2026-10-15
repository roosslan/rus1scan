// EventCallback.cpp: приёмник событий WIA (см. EventCallback.h).

#include "stdafx.h"
#include "EventCallback.h"
#include "strutil.h"

namespace WiaWrap
{
    // Создаёт приёмник событий. Счётчик ссылок начинается с 0 - объект нужно сразу
    // поместить в CComPtr.
    //   handler - функция, которой передаётся каждое пришедшее событие.
    CEventCallback::CEventCallback(WiaEventHandler handler)
        : m_cRef(0), m_handler(std::move(handler))
    {
    }

    // Стандартный IUnknown::QueryInterface: поддерживаются IUnknown и IWiaEventCallback.
    //   iid    - идентификатор запрашиваемого интерфейса;
    //   ppvObj - [out] указатель на интерфейс (с AddRef) или NULL.
    // Возвращает S_OK, E_NOINTERFACE или E_POINTER.
    STDMETHODIMP CEventCallback::QueryInterface(REFIID iid, LPVOID* ppvObj)
    {
        if (ppvObj == NULL)
        {
            return E_POINTER;
        }

        if (iid == IID_IUnknown)
        {
            *ppvObj = static_cast<IUnknown*>(this);
        }
        else if (iid == IID_IWiaEventCallback)
        {
            *ppvObj = static_cast<IWiaEventCallback*>(this);
        }
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    // Увеличивает счётчик ссылок (потокобезопасно). Возвращает новое значение.
    STDMETHODIMP_(ULONG) CEventCallback::AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    // Уменьшает счётчик ссылок; при достижении 0 удаляет объект. Возвращает новое значение.
    STDMETHODIMP_(ULONG) CEventCallback::Release()
    {
        const LONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

    // Вызывается службой WIA при наступлении события, на которое зарегистрирован объект.
    //   pEventGuid            - GUID события (WIA_EVENT_SCAN_IMAGE, WIA_EVENT_DEVICE_CONNECTED, ...);
    //   bstrEventDescription  - описание события;
    //   bstrDeviceID          - WIA ID устройства;
    //   bstrDeviceDescription - описание устройства;
    //   dwDeviceType          - тип устройства (STI_DEVICE_TYPE);
    //   bstrFullItemName      - полное имя элемента, к которому относится событие (может быть пустым);
    //   pulEventType          - тип события (WIA_NOTIFICATION_EVENT / WIA_ACTION_EVENT), может быть NULL;
    //   ulReserved            - зарезервировано.
    // Копирует параметры и передаёт их обработчику. Всегда возвращает S_OK.
    STDMETHODIMP CEventCallback::ImageEventCallback(
        LPCGUID pEventGuid,
        BSTR    bstrEventDescription,
        BSTR    bstrDeviceID,
        BSTR    bstrDeviceDescription,
        DWORD   dwDeviceType,
        BSTR    bstrFullItemName,
        ULONG*  pulEventType,
        ULONG   ulReserved
    )
    {
        UNREFERENCED_PARAMETER(ulReserved);

        WiaEventNotification event;
        if (pEventGuid != NULL)
        {
            event.eventGuid = *pEventGuid;
        }
        event.eventGuidText = strutil::GuidToString(event.eventGuid);
        event.eventDescription = strutil::BstrToWide(bstrEventDescription);
        event.deviceId = strutil::BstrToWide(bstrDeviceID);
        event.deviceDescription = strutil::BstrToWide(bstrDeviceDescription);
        event.fullItemName = strutil::BstrToWide(bstrFullItemName);
        event.deviceType = dwDeviceType;
        event.eventType = (pulEventType != NULL) ? *pulEventType : 0;

        LOG_SAVE << "WIA event '" << event.eventDescription << "' " << event.eventGuidText
                 << " from '" << event.deviceDescription << "' (" << event.deviceId << ") item='"
                 << event.fullItemName << "' type=" << event.eventType;

        if (m_handler)
        {
            m_handler(event);
        }
        return S_OK;
    }

    // Регистрирует приёмник на каждое из указанных событий для всех устройств
    // (идентификатор устройства NULL): так регистрация переживает отключение и
    // повторное подключение сканера, а нужное устройство выбирается по bstrDeviceID
    // в момент события. Ранее сделанные регистрации сохраняются.
    //   pWiaDevMgr - менеджер устройств WIA;
    //   events     - GUID событий.
    // Возвращает S_OK, если все события зарегистрированы, иначе код последней ошибки
    // (успешные регистрации при этом остаются в силе).
    HRESULT CEventCallback::Register(IWiaDevMgr* pWiaDevMgr, const std::vector<GUID>& events)
    {
        if (pWiaDevMgr == NULL)
        {
            return E_POINTER;
        }

        HRESULT result = S_OK;
        for (const GUID& guid : events)
        {
            CComPtr<IUnknown> registration;
            const HRESULT hr = pWiaDevMgr->RegisterEventCallbackInterface(0, NULL, &guid, this, &registration);
            if (SUCCEEDED(hr) && registration != NULL)
            {
                m_registrations.push_back(registration);
                LOG_SAVE << "Registered for WIA event " << strutil::GuidToString(guid);
            }
            else
            {
                result = FAILED(hr) ? hr : E_FAIL;
                LOG_SAVE << "RegisterEventCallbackInterface(" << strutil::GuidToString(guid)
                         << ") failed: " << rLogger::HrText(hr);
            }
        }
        return result;
    }

    // Отменяет все регистрации (освобождает объекты, полученные от
    // RegisterEventCallbackInterface). После этого новые события не приходят.
    void CEventCallback::Unregister()
    {
        if (!m_registrations.empty())
        {
            LOG_SAVE << "Unregistering " << m_registrations.size() << " WIA event registration(s)";
        }
        m_registrations.clear();
    }

    // Возвращает число действующих регистраций.
    size_t CEventCallback::RegisteredCount() const
    {
        return m_registrations.size();
    }

}; // namespace WiaWrap
