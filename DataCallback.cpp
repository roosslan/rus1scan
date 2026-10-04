/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Основано на примере Microsoft "WiaWrap" (см. DataCallback.h).

--*/

#include "stdafx.h"
#include "DataCallback.h"
#include "BitmapUtil.h"

namespace WiaWrap
{

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::CDataCallback
//
// Создаёт приёмник данных. Счётчик ссылок начинается с 0 - объект нужно сразу
// поместить в CComPtr (или вызвать AddRef).
//   pfnProgressCallback    - функция хода передачи; вызывается для каждого блока данных
//                            и статуса, не может быть NULL;
//   pProgressCallbackParam - параметр, передаваемый в pfnProgressCallback;
//   plCount                - [out] число полученных страниц (должно быть 0 на входе);
//   pppStream              - [out] массив IStream* с полученными страницами
//                            (выделяется через CoTaskMemRealloc, на входе NULL).
//

CDataCallback::CDataCallback(
    PFNPROGRESSCALLBACK  pfnProgressCallback,
    PVOID                pProgressCallbackParam,
    LONG                *plCount,
    IStream             ***pppStream
)
{
    m_cRef = 0;

    m_bBMP        = FALSE;
    m_nHeaderSize = 0;
    m_nDataSize   = 0;

    m_pfnProgressCallback    = pfnProgressCallback;
    m_pProgressCallbackParam = pProgressCallbackParam;

    m_plCount   = plCount;
    m_pppStream = pppStream;
}

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::QueryInterface
//
// Стандартный IUnknown::QueryInterface: поддерживаются IUnknown и IWiaDataCallback.
//   iid    - идентификатор запрашиваемого интерфейса;
//   ppvObj - [out] указатель на интерфейс (с AddRef) или NULL.
// Возвращает S_OK, E_NOINTERFACE или E_POINTER.
//

STDMETHODIMP CDataCallback::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    if (ppvObj == NULL)
    {
        return E_POINTER;
    }

    if (iid == IID_IUnknown)
    {
        *ppvObj = (IUnknown*) this;
    }
    else if (iid == IID_IWiaDataCallback)
    {
        *ppvObj = (IWiaDataCallback *) this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::AddRef
//
// Увеличивает счётчик ссылок (потокобезопасно). Возвращает новое значение.
//

STDMETHODIMP_(ULONG) CDataCallback::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::Release
//
// Уменьшает счётчик ссылок; при достижении 0 удаляет объект. Возвращает новое значение.
//

STDMETHODIMP_(ULONG) CDataCallback::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::BandedDataCallback
//
// Вызывается WIA во время передачи idtGetBandedData.
//   lReason          - причина вызова:
//                      IT_MSG_DATA_HEADER - заголовок передачи (формат и, возможно, размер);
//                      IT_MSG_DATA        - очередной блок данных (lOffset/lLength/pbBuffer);
//                      IT_MSG_STATUS      - только статус и процент выполнения;
//                      IT_MSG_NEW_PAGE / IT_MSG_TERMINATION - страница/передача завершена;
//   lStatus          - статус устройства (IT_STATUS_...);
//   lPercentComplete - процент выполнения;
//   lOffset          - смещение блока данных в изображении;
//   lLength          - длина блока данных;
//   lReserved        - зарезервировано;
//   lResLength       - зарезервировано;
//   pbBuffer         - данные блока (или WIA_DATA_CALLBACK_HEADER для IT_MSG_DATA_HEADER).
// Возвращает S_OK для продолжения, S_FALSE для отмены или код ошибки.
//

STDMETHODIMP CDataCallback::BandedDataCallback(
    LONG   lReason,
    LONG   lStatus,
    LONG   lPercentComplete,
    LONG   lOffset,
    LONG   lLength,
    LONG   lReserved,
    LONG   lResLength,
    PBYTE  pbBuffer
)
{
    UNREFERENCED_PARAMETER(lReserved);
    UNREFERENCED_PARAMETER(lResLength);

    HRESULT hr;

    switch (lReason)
    {
        case IT_MSG_DATA_HEADER:
        {
            PWIA_DATA_CALLBACK_HEADER pHeader = (PWIA_DATA_CALLBACK_HEADER) pbBuffer;

            if (pHeader == NULL)
            {
                return E_POINTER;
            }

            // Это передача BMP?

            m_bBMP = pHeader->guidFormatID == WiaImgFmt_MEMORYBMP || pHeader->guidFormatID == WiaImgFmt_BMP;

            // Для WiaImgFmt_MEMORYBMP WIA не передаёт BITMAPFILEHEADER - резервируем
            // под него место в начале буфера и заполним в конце страницы.

            m_nHeaderSize = pHeader->guidFormatID == WiaImgFmt_MEMORYBMP ? sizeof(BITMAPFILEHEADER) : 0;

            // Если размер изображения известен заранее - сразу выделяем буфер

            if (pHeader->lBufferSize != 0)
            {
                hr = ReAllocBuffer(m_nHeaderSize + pHeader->lBufferSize);

                if (FAILED(hr))
                {
                    return hr;
                }
            }

            break;
        }

        case IT_MSG_DATA:
        {
            hr = m_pfnProgressCallback(lStatus, lPercentComplete, m_pProgressCallbackParam);

            if (FAILED(hr) || hr == S_FALSE)
            {
                return hr;
            }

            // Первый блок BMP без заранее выделенного буфера: размер берём
            // из заголовка DIB в самом блоке.

            if (m_pStream == NULL && lOffset == 0 && m_bBMP)
            {
                LONG nBufferSize = BitmapUtil::GetBitmapSize(pbBuffer);

                if (nBufferSize != 0)
                {
                    hr = ReAllocBuffer(m_nHeaderSize + nBufferSize);

                    if (FAILED(hr))
                    {
                        return hr;
                    }
                }
            }

            // Если блок выходит за пределы буфера - расширяем буфер

            if (m_nHeaderSize + lOffset + lLength > m_nDataSize)
            {
                hr = ReAllocBuffer(m_nHeaderSize + lOffset + lLength);

                if (FAILED(hr))
                {
                    return hr;
                }
            }

            hr = CopyToBuffer(m_nHeaderSize + lOffset, pbBuffer, lLength);

            if (FAILED(hr))
            {
                return hr;
            }

            break;
        }

        case IT_MSG_STATUS:
        {
            hr = m_pfnProgressCallback(lStatus, lPercentComplete, m_pProgressCallbackParam);

            if (FAILED(hr) || hr == S_FALSE)
            {
                return hr;
            }

            break;
        }

        case IT_MSG_TERMINATION:
        case IT_MSG_NEW_PAGE:
        {
            if (m_pStream != NULL)
            {
                if (m_bBMP)
                {
                    // Поток создан через CreateStreamOnHGlobal, поэтому его память
                    // можно получить через GetHGlobalFromStream и поправить заголовок на месте.

                    HGLOBAL hBuffer;

                    hr = GetHGlobalFromStream(m_pStream, &hBuffer);

                    if (FAILED(hr))
                    {
                        return hr;
                    }

                    PBITMAPFILEHEADER pBuffer = (PBITMAPFILEHEADER) GlobalLock(hBuffer);

                    if (pBuffer == NULL)
                    {
                        return HRESULT_FROM_WIN32(GetLastError());
                    }

                    // Протяжные сканеры могут передать высоту 0 - вычисляем её

                    BitmapUtil::FixBitmapHeight(pBuffer + 1, m_nDataSize, TRUE);

                    // Для WiaImgFmt_MEMORYBMP заполняем зарезервированный BITMAPFILEHEADER

                    if (m_nHeaderSize != 0)
                    {
                        BitmapUtil::FillBitmapFileHeader(pBuffer + 1, pBuffer);
                    }

                    GlobalUnlock(hBuffer);
                }

                // Сохраняем страницу в массив готовых изображений

                hr = StoreBuffer();

                if (FAILED(hr))
                {
                    return hr;
                }
            }

            break;
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::ReAllocBuffer
//
// Устанавливает размер буфера текущей страницы; при первом вызове создаёт поток в памяти.
//   nSize - новый размер буфера в байтах.
// Возвращает S_OK или код ошибки.
//

HRESULT CDataCallback::ReAllocBuffer(ULONG nSize)
{
    HRESULT hr;

    if (m_pStream == 0)
    {
        hr = CreateStreamOnHGlobal(0, TRUE, &m_pStream);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    ULARGE_INTEGER liSize = { nSize };

    hr = m_pStream->SetSize(liSize);

    if (FAILED(hr))
    {
        return hr;
    }

    m_nDataSize = nSize;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::CopyToBuffer
//
// Записывает блок данных в буфер текущей страницы по указанному смещению.
//   nOffset - смещение в буфере;
//   pBuffer - данные;
//   nSize   - размер данных в байтах.
// Возвращает S_OK или код ошибки.
//

HRESULT CDataCallback::CopyToBuffer(ULONG nOffset, LPCVOID pBuffer, ULONG nSize)
{
    HRESULT hr;

    LARGE_INTEGER liOffset = { nOffset };

    hr = m_pStream->Seek(liOffset, STREAM_SEEK_SET, 0);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_pStream->Write(pBuffer, nSize, 0);

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback::StoreBuffer
//
// Добавляет готовую страницу в выходной массив (*m_pppStream, *m_plCount) и
// сбрасывает буфер, чтобы следующая страница начиналась с нового потока.
// Возвращает S_OK или E_OUTOFMEMORY.
//

HRESULT CDataCallback::StoreBuffer()
{
    IStream **ppStream = (IStream **) CoTaskMemRealloc(
        *m_pppStream,
        (*m_plCount + 1) * sizeof(IStream *)
    );

    if (ppStream == NULL)
    {
        return E_OUTOFMEMORY;
    }

    *m_pppStream = ppStream;

    // Перематываем поток в начало

    LARGE_INTEGER liZero = { 0 };

    m_pStream->Seek(liZero, STREAM_SEEK_SET, 0);

    // Кладём поток последним элементом массива

    (*m_pppStream)[*m_plCount] = m_pStream;
    (*m_pppStream)[*m_plCount]->AddRef();

    *m_plCount += 1;

    // Следующая страница начнётся с нового потока

    m_pStream.Release();

    m_nDataSize = 0;

    return S_OK;
}

}; // namespace WiaWrap
