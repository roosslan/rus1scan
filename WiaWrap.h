/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Основано на примере Microsoft "WiaWrap". Оставлены только функции, которые нужны
rus1scan: чтение/запись свойств WIA, подсчёт устройств и массив COM-указателей.
Функции с диалогами (WiaGetImage и др.) удалены - служба работает без окон.

--*/

#ifndef __WIAWRAP__
#define __WIAWRAP__

namespace WiaWrap
{

// Функция обратного вызова для отображения хода передачи данных.
//   lStatus          - IT_STATUS_TRANSFER_FROM_DEVICE / IT_STATUS_PROCESSING_DATA / IT_STATUS_TRANSFER_TO_CLIENT;
//   lPercentComplete - процент выполнения;
//   pParam           - произвольный параметр, переданный при создании CDataCallback.
// Вернуть S_OK - продолжить передачу, S_FALSE - отменить, код ошибки - прервать с ошибкой.
typedef HRESULT (CALLBACK *PFNPROGRESSCALLBACK)(
    LONG   lStatus,
    LONG   lPercentComplete,
    PVOID  pParam
);

// Возвращает количество WIA-устройств в системе.
HRESULT WiaGetNumDevices(IWiaDevMgr *pSuppliedWiaDevMgr, ULONG *pulNumDevices);

// Читает целочисленное свойство WIA.
HRESULT ReadPropertyLong(IWiaPropertyStorage *pWiaPropertyStorage, const PROPSPEC *pPropSpec, LONG *plResult);

// Читает свойство WIA типа GUID.
HRESULT ReadPropertyGuid(IWiaPropertyStorage *pWiaPropertyStorage, const PROPSPEC *pPropSpec, GUID *pguidResult);

// Читает строковое свойство WIA по его номеру.
HRESULT ReadPropertyString(IWiaPropertyStorage *pWiaPropertyStorage, PROPID propId, std::wstring *pResult);

// Читает наибольшее допустимое значение свойства WIA с диапазоном (WIA_PROP_RANGE).
HRESULT ReadPropertyRangeMax(IWiaPropertyStorage *pWiaPropertyStorage, PROPID propId, LONG *plMax);

// Записывает целочисленное свойство WIA по его номеру.
HRESULT WritePropertyLong(IWiaPropertyStorage *pWiaPropertyStorage, PROPID propId, LONG lValue);

// Записывает свойство WIA типа GUID по его номеру.
HRESULT WritePropertyGuid(IWiaPropertyStorage *pWiaPropertyStorage, PROPID propId, const GUID &guidValue);

//////////////////////////////////////////////////////////////////////////
//
// CComPtrArray
//
// Массив COM-указателей, выделенный через CoTaskMemAlloc (в таком виде WIA и
// CDataCallback возвращают массивы IStream*/IWiaItem*). В деструкторе освобождает
// каждый элемент (Release) и сам массив (CoTaskMemFree).
//

template <class T>
class CComPtrArray
{
public:
    // Создаёт пустой массив.
    CComPtrArray()
    {
        m_pArray = NULL;
        m_nCount = 0;
    }

    // Создаёт массив из nCount пустых (NULL) элементов.
    //   nCount - количество элементов.
    explicit CComPtrArray(int nCount)
    {
        m_pArray = (T **) CoTaskMemAlloc(nCount * sizeof(T *));

        m_nCount = m_pArray == NULL ? 0 : nCount;

        for (int i = 0; i < m_nCount; ++i)
        {
            m_pArray[i] = NULL;
        }
    }

    // Создаёт копию массива (элементы получают AddRef).
    //   rhs - копируемый массив.
    CComPtrArray(const CComPtrArray& rhs)
    {
        Copy(rhs);
    }

    // Освобождает элементы и память массива.
    ~CComPtrArray()
    {
        Clear();
    }

    // Заменяет содержимое копией другого массива.
    //   rhs - копируемый массив.
    CComPtrArray &operator =(const CComPtrArray& rhs)
    {
        if (this != &rhs)
        {
            Clear();
            Copy(rhs);
        }

        return *this;
    }

    // Возвращает указатель на первый элемент массива.
    operator T **()
    {
        return m_pArray;
    }

    // Возвращает true, если массив не выделен.
    bool operator!()
    {
        return m_pArray == NULL;
    }

    // Возвращает адрес внутреннего указателя - для передачи в функции,
    // которые сами выделяют массив (параметр вида T***).
    T ***operator&()
    {
        return &m_pArray;
    }

    // Возвращает ссылку на счётчик элементов (заполняется вместе с массивом).
    LONG &Count()
    {
        return m_nCount;
    }

    // Освобождает все элементы (Release) и память массива; массив становится пустым.
    void Clear()
    {
        if (m_pArray != NULL)
        {
            for (int i = 0; i < m_nCount; ++i)
            {
                if (m_pArray[i] != NULL)
                {
                    m_pArray[i]->Release();
                }
            }

            CoTaskMemFree(m_pArray);

            m_pArray = NULL;
            m_nCount = 0;
        }
    }

    // Копирует элементы другого массива в новый массив (с AddRef каждого элемента).
    // Текущее содержимое должно быть уже освобождено.
    //   rhs - копируемый массив.
    void Copy(const CComPtrArray& rhs)
    {
        m_pArray = NULL;
        m_nCount = 0;

        if (rhs.m_pArray != NULL)
        {
            m_pArray = (T**) CoTaskMemAlloc(rhs.m_nCount * sizeof(T *));

            if (m_pArray != NULL)
            {
                m_nCount = rhs.m_nCount;

                for (int i = 0; i < m_nCount; ++i)
                {
                    m_pArray[i] = rhs.m_pArray[i];

                    if (m_pArray[i] != NULL)
                    {
                        m_pArray[i]->AddRef();
                    }
                }
            }
        }
    }

private:
    T    **m_pArray;
    LONG  m_nCount;
};

}; // namespace WiaWrap

#endif //__WIAWRAP__
