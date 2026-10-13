/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Основано на примере Microsoft "WiaWrap": функции для работы с DIB в памяти.

--*/

#include "stdafx.h"
#include "BitmapUtil.h"

namespace BitmapUtil
{

//////////////////////////////////////////////////////////////////////////
//
// GetBitmapHeaderSize
//
// Определяет размер заголовка DIB по его первому полю (biSize/bcSize).
//   pDib - указатель на DIB в памяти (начинается с BITMAPCOREHEADER,
//          BITMAPINFOHEADER, BITMAPV4HEADER или BITMAPV5HEADER).
// Возвращает размер заголовка в байтах или 0, если заголовок не распознан.
//

ULONG GetBitmapHeaderSize(LPCVOID pDib)
{
    ULONG nHeaderSize = *(PDWORD) pDib;

    switch (nHeaderSize)
	{
		case sizeof(BITMAPCOREHEADER):
        case sizeof(BITMAPINFOHEADER):
		case sizeof(BITMAPV4HEADER):
		case sizeof(BITMAPV5HEADER):
		{
			return nHeaderSize;
		}
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
//
// GetBitmapLineWidthInBytes
//
// Вычисляет размер одной строки пикселей в байтах. Строки DIB выровнены на 4 байта.
//   nWidthInPixels - ширина строки в пикселях;
//   nBitCount      - число бит на пиксель (1, 4, 8, 16, 24, 32).
// Возвращает размер строки в байтах.
//

ULONG GetBitmapLineWidthInBytes(ULONG nWidthInPixels, ULONG nBitCount)
{
    return (((nWidthInPixels * nBitCount) + 31) & ~31) >> 3;
}


//////////////////////////////////////////////////////////////////////////
//
// GetBitmapDimensions
//
// Возвращает ширину и высоту DIB в пикселях.
//   pDib    - указатель на DIB в памяти;
//   pWidth  - [out, необязательный] ширина; может быть NULL;
//   pHeight - [out, необязательный] высота (всегда положительная); может быть NULL.
// Возвращает TRUE, если заголовок распознан, иначе FALSE.
//

BOOL GetBitmapDimensions(LPCVOID pDib, UINT *pWidth, UINT *pHeight)
{
    ULONG nHeaderSize = GetBitmapHeaderSize(pDib);

    if (nHeaderSize == 0)
    {
        return FALSE;
    }

    if (nHeaderSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER pbmch = (PBITMAPCOREHEADER) pDib;

        if (pWidth != NULL)
        {
            *pWidth  = pbmch->bcWidth;
        }

        if (pHeight != NULL)
        {
            *pHeight = pbmch->bcHeight;
        }
    }
    else
    {
        PBITMAPINFOHEADER pbmih = (PBITMAPINFOHEADER) pDib;

        if (pWidth != NULL)
        {
            *pWidth  = pbmih->biWidth;
        }

        if (pHeight != NULL)
        {
            *pHeight = abs(pbmih->biHeight);
        }
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
// GetBitmapSize
//
// Вычисляет полный размер DIB: заголовок + палитра (если есть) + маски BI_BITFIELDS +
// ICC-профиль (если есть) + пиксельные данные. Используется, чтобы заранее выделить
// буфер под изображение при передаче данных от сканера.
//   pDib - указатель на DIB в памяти.
// Возвращает размер в байтах или 0, если заголовок не распознан.
//

ULONG GetBitmapSize(LPCVOID pDib)
{
    ULONG nHeaderSize = GetBitmapHeaderSize(pDib);

    if (nHeaderSize == 0)
    {
        return 0;
    }

    // Start the calculation with the header size

    ULONG nDibSize = nHeaderSize;

    // is this an old style BITMAPCOREHEADER?

    if (nHeaderSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER pbmch = (PBITMAPCOREHEADER) pDib;

        // Add the color table size

        if (pbmch->bcBitCount <= 8)
        {
            nDibSize += sizeof(RGBTRIPLE) * (1i64 << pbmch->bcBitCount);
        }

        // Add the bitmap size

        ULONG nWidth = GetBitmapLineWidthInBytes(pbmch->bcWidth, pbmch->bcBitCount);

        nDibSize += nWidth * pbmch->bcHeight;
    }
    else
    {
        // this is at least a BITMAPINFOHEADER

        PBITMAPINFOHEADER pbmih = (PBITMAPINFOHEADER) pDib;

        // Add the color table size

        if (pbmih->biClrUsed != 0)
        {
            nDibSize += sizeof(RGBQUAD) * pbmih->biClrUsed;
        }
        else if (pbmih->biBitCount <= 8)
        {
            nDibSize += sizeof(RGBQUAD) * (1i64 << pbmih->biBitCount);
        }

        // Add the bitmap size

        if (pbmih->biSizeImage != 0)
        {
            nDibSize += pbmih->biSizeImage;
        }
        else
        {
            // biSizeImage must be specified for compressed bitmaps

            if (pbmih->biCompression != BI_RGB &&
                pbmih->biCompression != BI_BITFIELDS)
            {
                return 0;
            }

            ULONG nWidth = GetBitmapLineWidthInBytes(pbmih->biWidth, pbmih->biBitCount);

            nDibSize += nWidth * abs(pbmih->biHeight);
        }

        // Consider special cases

        if (nHeaderSize == sizeof(BITMAPINFOHEADER))
        {     
            // If this is a 16 or 32 bit bitmap and BI_BITFIELDS is used, 
            // bmiColors member contains three DWORD color masks.
            // For V4 or V5 headers, this info is included the header

            if (pbmih->biCompression == BI_BITFIELDS)
            {
                nDibSize += 3 * sizeof(DWORD);
            }
        }
        else if (nHeaderSize >= sizeof(BITMAPV5HEADER))
        {
            // If this is a V5 header and an ICM profile is specified,
            // we need to consider the profile data size
            
            PBITMAPV5HEADER pbV5h = (PBITMAPV5HEADER) pDib;

            // if there is some padding before the profile data, add it

            if (pbV5h->bV5ProfileData > nDibSize)
            {
                nDibSize = pbV5h->bV5ProfileData;
            }

            // add the profile data size

            nDibSize += pbV5h->bV5ProfileSize;
        }
    }

    return nDibSize;
}


//////////////////////////////////////////////////////////////////////////
//
// GetBitmapOffsetBits
//
// Вычисляет смещение пиксельных данных от начала DIB (заголовок + палитра + маски + профиль).
//   pDib - указатель на DIB в памяти.
// Возвращает смещение в байтах или 0, если заголовок не распознан.
//

ULONG GetBitmapOffsetBits(LPCVOID pDib)
{
    ULONG nHeaderSize = GetBitmapHeaderSize(pDib);

    if (nHeaderSize == 0)
    {
        return 0;
    }

    // Start the calculation with the header size

    ULONG nOffsetBits = nHeaderSize;

    // is this an old style BITMAPCOREHEADER?

    if (nHeaderSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER pbmch = (PBITMAPCOREHEADER) pDib;

        // Add the color table size

        if (pbmch->bcBitCount <= 8)
        {
            nOffsetBits += sizeof(RGBTRIPLE) * (1i64 << pbmch->bcBitCount);
        }
    }
    else
    {
        // this is at least a BITMAPINFOHEADER

        PBITMAPINFOHEADER pbmih = (PBITMAPINFOHEADER) pDib;

        // Add the color table size

        if (pbmih->biClrUsed != 0)
        {
            nOffsetBits += sizeof(RGBQUAD) * pbmih->biClrUsed;
        }
        else if (pbmih->biBitCount <= 8)
        {
            nOffsetBits += sizeof(RGBQUAD) * (1i64 << pbmih->biBitCount);
        }

        // Consider special cases

        if (nHeaderSize == sizeof(BITMAPINFOHEADER))
        {     
            // If this is a 16 or 32 bit bitmap and BI_BITFIELDS is used, 
            // bmiColors member contains three DWORD color masks.
            // For V4 or V5 headers, this info is included in the header

            if (pbmih->biCompression == BI_BITFIELDS)
            {
                nOffsetBits += 3 * sizeof(DWORD);
            }
        }
        else if (nHeaderSize >= sizeof(BITMAPV5HEADER))
        {
            // If this is a V5 header and an ICM profile is specified,
            // we need to consider the profile data size
            
            PBITMAPV5HEADER pbV5h = (PBITMAPV5HEADER) pDib;

            // if the profile data comes before the pixel data, add it

            if (pbV5h->bV5ProfileData <= nOffsetBits)
            {
                nOffsetBits += pbV5h->bV5ProfileSize;
            }
        }
    }

    return nOffsetBits;
}


//////////////////////////////////////////////////////////////////////////
//
// FixBitmapHeight
//
// Некоторые протяжные сканеры передают высоту изображения 0, потому что заранее её
// не знают. Функция вычисляет высоту по фактическому размеру данных и исправляет
// поля biHeight/biSizeImage (bcHeight для старого заголовка).
//   pDib     - указатель на DIB в памяти (заголовок изменяется на месте);
//   nSize    - полный размер данных изображения в байтах;
//   bTopDown - TRUE, если первая строка в памяти - верхняя строка изображения
//              (тогда высота записывается отрицательной).
// Возвращает TRUE, если заголовок распознан, иначе FALSE.
//

BOOL FixBitmapHeight(PVOID pDib, ULONG nSize, BOOL bTopDown)
{
    ULONG nHeaderSize = GetBitmapHeaderSize(pDib);

    if (nHeaderSize == 0)
    {
        return FALSE;
    }

    // is this an old style BITMAPCOREHEADER?

    if (nHeaderSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER pbmch = (PBITMAPCOREHEADER) pDib;

        // fix the height value if necessary

        if (pbmch->bcHeight == 0)
        {
            // start the calculation with the header size

            ULONG nSizeImage = nSize - nHeaderSize;

            // subtract the color table size

            if (pbmch->bcBitCount <= 8)
            {
                nSizeImage -= sizeof(RGBTRIPLE) * (1i64 << pbmch->bcBitCount);
            }

            // calculate the height

            ULONG nWidth = GetBitmapLineWidthInBytes(pbmch->bcWidth, pbmch->bcBitCount);

            if (nWidth == 0)
            {
                return FALSE;
            }

            LONG nHeight = nSizeImage / nWidth;

            pbmch->bcHeight = (WORD) nHeight;
        }
    }
    else
    {
        // this is at least a BITMAPINFOHEADER

        PBITMAPINFOHEADER pbmih = (PBITMAPINFOHEADER) pDib;

        // fix the height value if necessary

        if (pbmih->biHeight == 0)
        {
            // find the size of the image data

            ULONG nSizeImage;

            if (pbmih->biSizeImage != 0)
            {
                // if the size is specified in the header, take it

                nSizeImage = pbmih->biSizeImage;
            }
            else
            {
                // start the calculation with the header size

                nSizeImage = nSize - nHeaderSize;

                // subtract the color table size

                if (pbmih->biClrUsed != 0)
                {
                    nSizeImage -= sizeof(RGBQUAD) * pbmih->biClrUsed;
                }
                else if (pbmih->biBitCount <= 8)
                {
                    nSizeImage -= sizeof(RGBQUAD) * (1i64 << pbmih->biBitCount);
                }

                // Consider special cases

                if (nHeaderSize == sizeof(BITMAPINFOHEADER))
                {     
                    // If this is a 16 or 32 bit bitmap and BI_BITFIELDS is used, 
                    // bmiColors member contains three DWORD color masks.
                    // For V4 or V5 headers, this info is included the header

                    if (pbmih->biCompression == BI_BITFIELDS)
                    {
                        nSizeImage -= 3 * sizeof(DWORD);
                    }
                }
                else if (nHeaderSize >= sizeof(BITMAPV5HEADER))
                {
                    // If this is a V5 header and an ICM profile is specified,
                    // we need to consider the profile data size
            
                    PBITMAPV5HEADER pbV5h = (PBITMAPV5HEADER) pDib;

                    // add the profile data size

                    nSizeImage -= pbV5h->bV5ProfileSize;
                }

                // store the image size

                pbmih->biSizeImage = nSizeImage;
            }

            // finally, calculate the height

            ULONG nWidth = GetBitmapLineWidthInBytes(pbmih->biWidth, pbmih->biBitCount);

            if (nWidth == 0)
            {
                return FALSE;
            }

			LONG nHeight = nSizeImage / nWidth;

            pbmih->biHeight = bTopDown ? -nHeight : nHeight;
        }
    }

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
// FillBitmapFileHeader
//
// Заполняет BITMAPFILEHEADER (заголовок .bmp-файла) по данным DIB. Нужна для формата
// WiaImgFmt_MEMORYBMP, в котором WIA передаёт DIB без файлового заголовка.
//   pDib  - указатель на DIB в памяти;
//   pbmfh - [out] заполняемый заголовок файла.
// Возвращает TRUE, если заголовок DIB распознан, иначе FALSE.
//

BOOL FillBitmapFileHeader(LPCVOID pDib, PBITMAPFILEHEADER pbmfh)
{
    ULONG nSize = GetBitmapSize(pDib);

    if (nSize == 0)
    {
        return FALSE;
    }

    ULONG nOffset = GetBitmapOffsetBits(pDib);

    if (nOffset == 0)
    {
        return FALSE;
    }

    pbmfh->bfType      = MAKEWORD('B', 'M');
    pbmfh->bfSize      = sizeof(BITMAPFILEHEADER) + nSize;
    pbmfh->bfReserved1 = 0;
    pbmfh->bfReserved2 = 0;
    pbmfh->bfOffBits   = sizeof(BITMAPFILEHEADER) + nOffset;

    return TRUE;
}

}; // namespace BitmapUtil
