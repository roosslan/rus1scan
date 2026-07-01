#include "stdafx.h"

class printer_select;

class image_print {
    DOCINFO di_;

    LPBITMAPINFO info_;           // Struct for storing the DIB information,
							      // it will be used by 'StretchDIBits()'
    HBITMAP      hbit_;           // Handle to the bitmap to print
    BITMAP       bm_;             // Struct used for obtaining information
								  // about the bitmap (size, color depth...)
    int          n_colors_;       // store the number of colors the DIB has
    int          sizeinfo_;       // Will contain the size of info
    RGBQUAD      rgb_[256];       // Used to store the DIB color table
    int          your_bitmap_id_;

public:
    explicit image_print(LPCTSTR file_name);
    int image_print::print(const HDC &pDC) const;
    ~image_print();
};