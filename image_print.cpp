#include "stdafx.h"
#include "image_print.h"

image_print::image_print(const LPCTSTR file_name) {

    hbit_ = static_cast<HBITMAP>(LoadImage(nullptr, file_name, IMAGE_BITMAP, 0, 0,
                                          LR_CREATEDIBSECTION | LR_LOADFROMFILE));

    GetObject(hbit_, sizeof(BITMAP), (LPVOID)&bm_);

    /* the number of colors the DIB has */
    int n_colors = (1 << bm_.bmBitsPixel);

    /* if DIB is 24 bit */
    if (n_colors > 256)
        n_colors = 0;


    /* Color table is only needed when the DIB has 256 colors or less */
    sizeinfo_ = n_colors * sizeof(RGBQUAD) + sizeof(BITMAPINFO);
    /* storing info in memory */
    info_ = static_cast<LPBITMAPINFO>(malloc(sizeinfo_));

    /* Before 'StretchDIBits()' we have to fill LPBITMAPINFO
     * This information was stored in 'bm'.
	 */
    info_->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info_->bmiHeader.biWidth = bm_.bmWidth;
    info_->bmiHeader.biHeight = bm_.bmHeight;
    info_->bmiHeader.biPlanes = 1;
    info_->bmiHeader.biBitCount = bm_.bmBitsPixel * bm_.bmPlanes;
    info_->bmiHeader.biCompression = BI_RGB;
    info_->bmiHeader.biSizeImage = bm_.bmWidthBytes * bm_.bmHeight;
    info_->bmiHeader.biXPelsPerMeter = 0;
    info_->bmiHeader.biYPelsPerMeter = 0;
    info_->bmiHeader.biClrUsed = 0;
    info_->bmiHeader.biClrImportant = 0;

    di_.cbSize = sizeof(DOCINFO);
    di_.lpszDocName = file_name;
    di_.lpszOutput = static_cast<LPTSTR>(nullptr);

    if (n_colors <= 256) {
	    const HDC hMemDC = CreateCompatibleDC(nullptr);

        const auto hOldBitmap = static_cast<HBITMAP>(SelectObject(hMemDC, hbit_));
        GetDIBColorTable(hMemDC, 0, n_colors, rgb_);

        /* pass this color information to LPBITMAPINFO */
        for (int i_cnt = 0; i_cnt < n_colors; ++i_cnt)
        {
            info_->bmiColors[i_cnt].rgbRed   = rgb_[i_cnt].rgbRed;
            info_->bmiColors[i_cnt].rgbGreen = rgb_[i_cnt].rgbGreen;
            info_->bmiColors[i_cnt].rgbBlue  = rgb_[i_cnt].rgbBlue;
        }

        SelectObject(hMemDC, hOldBitmap);
        DeleteDC(hMemDC);
    }
}

image_print::~image_print() {
    DeleteObject(hbit_);
}


int image_print::print(const HDC &pDC) const {
    //'StretchDIBiting'! 'pDC' is the printer DC
    HDC hdc = CreateCompatibleDC(pDC);

    StartDoc(pDC, &di_);
    StartPage(pDC);

    StretchDIBits(pDC,
        0,                        //initial_pos_x
        0,                        //initial_pos_y
        info_->bmiHeader.biWidth,  //size_x
        info_->bmiHeader.biHeight, //size_y
        0,
        0,
        bm_.bmWidth,
        bm_.bmHeight,
        bm_.bmBits,
        info_,
        DIB_RGB_COLORS,
        SRCCOPY);

    EndPage(pDC);
    EndDoc(pDC);
    return 0;
}

