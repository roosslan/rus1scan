// EventCallback.h: приёмник событий WIA (нажатия кнопок сканера, подключение/отключение
// устройств), регистрируемый во время работы через IWiaDevMgr::RegisterEventCallbackInterface.

#ifndef __EVENTCALLBACK__
#define __EVENTCALLBACK__

#include <functional>
#include <string>
#include <vector>

// Данные пришедшего события WIA (копия параметров ImageEventCallback).
struct WiaEventNotification
{
    GUID         eventGuid = GUID_NULL;
    std::wstring eventGuidText;         // "{...}"
    std::wstring eventDescription;
    std::wstring deviceId;
    std::wstring deviceDescription;
    std::wstring fullItemName;          // Элемент, к которому относится событие (часто пусто)
    DWORD        deviceType = 0;
    ULONG        eventType = 0;         // WIA_NOTIFICATION_EVENT / WIA_ACTION_EVENT
};

// Обработчик событий. Вызывается в потоке RPC службы WIA - не должен выполнять
// долгих операций (сканирование), только поставить событие в очередь.
using WiaEventHandler = std::function<void(const WiaEventNotification&)>;

namespace WiaWrap
{

class CEventCallback : public IWiaEventCallback
{
public:
    // Создаёт приёмник событий (см. EventCallback.cpp).
    explicit CEventCallback(WiaEventHandler handler);

    // IUnknown

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IWiaEventCallback

    STDMETHOD(ImageEventCallback)(
        LPCGUID pEventGuid,
        BSTR    bstrEventDescription,
        BSTR    bstrDeviceID,
        BSTR    bstrDeviceDescription,
        DWORD   dwDeviceType,
        BSTR    bstrFullItemName,
        ULONG  *pulEventType,
        ULONG   ulReserved
    );

    // Регистрирует приёмник на указанные события всех устройств.
    HRESULT Register(IWiaDevMgr* pWiaDevMgr, const std::vector<GUID>& events);

    // Отменяет все регистрации.
    void Unregister();

    // Число успешных регистраций.
    size_t RegisteredCount() const;

protected:
    ~CEventCallback() = default;    // Удаляется только через Release()

private:
    LONG                           m_cRef;
    WiaEventHandler                 m_handler;
    std::vector<CComPtr<IUnknown>>  m_registrations;   // Освобождение объекта = отмена регистрации
};

}; // namespace WiaWrap

#endif //__EVENTCALLBACK__
