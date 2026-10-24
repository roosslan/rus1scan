// rus1scan.h: работа с устройствами WIA - перечисление устройств и их событий (кнопок)
// и получение изображения со сканера без пользовательского интерфейса.

#pragma once

#include <string>
#include <vector>

#include "config.h"

// Событие (кнопка) устройства WIA.
struct WiaEventInfo
{
    std::wstring guid;          // GUID события "{...}"
    std::wstring name;          // Имя события от драйвера
    std::wstring description;   // Описание события от драйвера
    ULONG        flags = 0;     // WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT
};

// Устройство WIA.
struct WiaDeviceInfo
{
    std::wstring id;            // WIA_DIP_DEV_ID
    std::wstring name;          // WIA_DIP_DEV_NAME
    std::wstring description;   // WIA_DIP_DEV_DESC
    LONG         type = 0;      // WIA_DIP_DEV_TYPE
    bool         online = true; // false - устройство известно только из конфига
    std::vector<WiaEventInfo> events;
};

// Одна отсканированная страница - полный .bmp-файл в памяти (с BITMAPFILEHEADER).
struct ScannedImage
{
    std::vector<BYTE> bmp;
};

// Операции с WIA. Все методы статические; перед вызовом в потоке должен быть
// инициализирован COM (CoInitializeEx).
class CRus1scan
{
public:
    // Подключается к менеджеру устройств WIA.
    static HRESULT CreateDeviceManager(CComPtr<IWiaDevMgr>& manager);

    // Перечисляет устройства WIA вместе с их событиями (при необходимости - и отключённые).
    static HRESULT EnumerateDevices(std::vector<WiaDeviceInfo>& devices, bool includeDisconnected = false);

    // Читает список событий (кнопок) устройства.
    static HRESULT ReadDeviceEvents(IWiaItem* rootItem, std::vector<WiaEventInfo>& events);

    // Сканирует изображение(я) с устройства без пользовательского интерфейса.
    static HRESULT AcquireImages(const std::wstring& deviceId, const std::wstring& itemName,
                                 const WiaPropList& wiaProps, std::vector<ScannedImage>& images);

private:
    static HRESULT FindTransferItem(IWiaItem* rootItem, const std::wstring& itemName, CComPtr<IWiaItem>& item);
    static HRESULT FindFirstImageItem(IWiaItem* parent, int depth, CComPtr<IWiaItem>& item);
    static void    ApplyWiaProperties(IWiaItem* rootItem, IWiaItem* item, const WiaPropList& wiaProps);
    static void    EnableFeederAllPages(IWiaItem* rootItem);
    static HRESULT TransferItem(IWiaItem* item, std::vector<ScannedImage>& images);
    static HRESULT CALLBACK TransferProgress(LONG lStatus, LONG lPercentComplete, PVOID pParam);
};
