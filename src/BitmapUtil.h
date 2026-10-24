/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Основано на примере Microsoft "WiaWrap". Вспомогательные функции для работы
с DIB (device-independent bitmap) в памяти. Подробные описания - в BitmapUtil.cpp.

--*/

#ifndef __BITMAPUTIL__
#define __BITMAPUTIL__

namespace BitmapUtil
{

// Размер заголовка DIB в байтах (0, если заголовок не распознан).
ULONG GetBitmapHeaderSize(LPCVOID pDib);

// Размер одной строки изображения в байтах (с выравниванием на 4 байта).
ULONG GetBitmapLineWidthInBytes(ULONG nWidthInPixels, ULONG nBitCount);

// Ширина и высота DIB в пикселях.
BOOL GetBitmapDimensions(LPCVOID pDib, UINT *pWidth, UINT *pHeight);

// Полный размер DIB: заголовок + палитра + ICC-профиль + пиксели.
ULONG GetBitmapSize(LPCVOID pDib);

// Смещение пиксельных данных от начала DIB.
ULONG GetBitmapOffsetBits(LPCVOID pDib);

// Вычисляет высоту изображения, если сканер передал 0 (протяжные сканеры).
BOOL FixBitmapHeight(PVOID pDib, ULONG nSize, BOOL bTopDown);

// Заполняет BITMAPFILEHEADER по данным DIB.
BOOL FillBitmapFileHeader(LPCVOID pDib, PBITMAPFILEHEADER pbmfh);

}; // namespace BitmapUtil

#endif //__BITMAPUTIL__
