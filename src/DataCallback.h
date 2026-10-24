/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Основано на примере Microsoft "WiaWrap".

--*/

#ifndef __DATACALLBACK__
#define __DATACALLBACK__

#include "WiaWrap.h"

namespace WiaWrap
{

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback
//
// Приёмник данных WIA для передачи idtGetBandedData (TYMED_CALLBACK).
// Складывает каждую полученную страницу в отдельный IStream в памяти; для BMP
// дописывает недостающий BITMAPFILEHEADER, так что каждый поток содержит готовый
// .bmp-файл. Готовые потоки добавляются в массив, указанный при создании объекта.
//

class CDataCallback : public IWiaDataCallback
{
public:
    // Создаёт приёмник данных (см. DataCallback.cpp).
    CDataCallback(
        PFNPROGRESSCALLBACK  pfnProgressCallback,
        PVOID                pProgressCallbackParam,
        LONG                *plCount,
        IStream             ***pppStream
    );

    // IUnknown

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IWiaDataCallback

    STDMETHOD(BandedDataCallback) (
        LONG  lReason,
        LONG  lStatus,
        LONG  lPercentComplete,
        LONG  lOffset,
        LONG  lLength,
        LONG  lReserved,
        LONG  lResLength,
        PBYTE pbBuffer
    );

private:
    HRESULT ReAllocBuffer(ULONG nBufferSize);
    HRESULT CopyToBuffer(ULONG nOffset, LPCVOID pBuffer, ULONG nSize);
    HRESULT StoreBuffer();

private:
    LONG m_cRef;

    BOOL              m_bBMP;           // Передаётся BMP (нужна проверка/дополнение заголовка)
    LONG              m_nHeaderSize;    // Сколько байт зарезервировано под BITMAPFILEHEADER
    LONG              m_nDataSize;      // Текущий размер буфера страницы
    CComPtr<IStream>  m_pStream;        // Буфер текущей страницы

    PFNPROGRESSCALLBACK  m_pfnProgressCallback;
    PVOID                m_pProgressCallbackParam;

    LONG    *m_plCount;                 // Куда писать число готовых страниц
    IStream ***m_pppStream;             // Куда складывать готовые страницы
};

}; // namespace WiaWrap

#endif //__DATACALLBACK__
