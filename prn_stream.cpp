#undef  UNICODE
#undef _UNICODE

#include "prn_stream.h"

#include <windows.h>
#include <tchar.h>
#include <iostream>

namespace pst {

    pstream::pstream() : left_margin_(),right_margin_(),top_margin_(),bottom_margin_(),page_numbers_(false),tab_size_(4) {

        DWORD buff_size;

        GetDefaultPrinter(0, &buff_size);
        const auto pPrinterName = new _TCHAR[buff_size]();
        GetDefaultPrinter (pPrinterName,&buff_size);
        defaultPrinter =  pPrinterName;
        
        stream_status_ = (defaultPrinter.empty())? fail : good;
    }

    
    pstream::pstream (const std::string& which_printer) : left_margin_(), right_margin_(), top_margin_ (), bottom_margin_(), page_numbers_(false), tab_size_(4) {
       defaultPrinter = which_printer;
       stream_status_ = (defaultPrinter.empty())? fail : good;     
   }    
    
    pstream::~pstream() { }

    pstream& pstream::operator <<(std::string name) {
        if (! stream_status_) {
            /* replace all tabs by the required number of spaces */
            size_t position = 0;

            while (position != std::string::npos) {
                
                if ((position = name.find('\t', position)) != std::string::npos) {
                    name.replace(position,1,std::string(tab_size_,' ') );
                }
            }

            /* split the string up at the new line char */
            position = 0;

            do {
                size_t foundPos = name.find('\n', position);
                std::string str = name.substr(position,foundPos-position);
                lines.push_back(str);
                position = foundPos+1;
            } while (position != 0);
        }
        
        return *this; 
    }   
    
    pstream& pstream::operator <<(char * name) {
        if (!stream_status_)
            operator<< (std::string(name));

        return *this;
    }


    pstream& pstream::operator << (std::ifstream & in_file) {
        if (!stream_status_) {
            if (in_file) {
                while (in_file) {
                    std::string str;
                    std::getline(in_file,str);

                    *this  << str ;
                }
            }
        }
        return *this;
    }



    pstream& pstream::operator <<(pstream &(*ptr)(pstream &)) {
        ptr(*this);
        return *this;
    }


    pstream& flushp (pstream& p) {
        
        if (p.lines.empty() || p.stream_status_)
            return p;

        HDC pdc;
        pdc = CreateDC("winspool", p.defaultPrinter.c_str(), nullptr, nullptr);

    	int log_pixels_x = GetDeviceCaps(pdc,LOGPIXELSX);
        int log_pixels_y = GetDeviceCaps(pdc, LOGPIXELSY);
        int vRes = GetDeviceCaps(pdc, VERTRES);
        int hRes = GetDeviceCaps(pdc, HORZRES);
        int scaleX = GetDeviceCaps(pdc, SCALINGFACTORX);

        TEXTMETRIC tm;
        GetTextMetrics(pdc, &tm);
        long aveCharWidth= tm.tmAveCharWidth;
        long maxCharWidth= tm.tmMaxCharWidth;
        long char_height = tm.tmHeight;
        long line_height = char_height;// + tm.tmExternalLeading;

        /* we need to adjust for the margins (change mm to pixels) */
        int hPixelsMM = log_pixels_x/25;
        int vPixelsMM = log_pixels_y/25;

        int left_start = ( (p.left_margin_*hPixelsMM) > hRes/4)? hRes/4: p.left_margin_*hPixelsMM;
        int right_end =  ( (p.right_margin_*hPixelsMM) > hRes/4)? hRes - hRes/4: hRes - p.right_margin_*hPixelsMM;
        int top_start = ( (p.top_margin_*vPixelsMM) > vRes/4)? vRes/4: p.top_margin_*vPixelsMM;
        int bottom_end = ( (p.bottom_margin_ *vPixelsMM) > vRes/4)? vRes - vRes/4: vRes - p.bottom_margin_*vPixelsMM;

        /* Use 10 point courier font */
        auto hFont = CreateFont(-(log_pixels_y/72)*10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FIXED_PITCH, "courier");
        auto hOldFont = static_cast<HFONT>(SelectObject(pdc, hFont));
        

        /* start document */
        DOCINFO doc_info = {0};
        doc_info.cbSize = sizeof(doc_info);

        StartDoc(pdc,&doc_info);

        /* start the pages */
        int cur_pos_y = top_start+line_height; 
        int cur_pos_x= left_start;

        /* used for word breaking checks */
        int last_space_pos;

        auto iter = p.lines.begin(), iterEnd = p.lines.end();

        /* Start the first page */
        StartPage(pdc);
        
        for (; iter != iterEnd; ++iter) {
            std::string cur_str = *iter;
            std::string temp_str;
            
            /* for empty lines we will  make the line one space long */
            if(cur_str.empty())
                cur_str = " ";

            last_space_pos = 0;
            for (int start_index = 0, count =0; start_index < cur_str.length() ; count ++) {

                int char_width;
                char c = cur_str[count];
                GetCharWidth32(pdc, cur_str[count], cur_str[count], &char_width);

                if (count < cur_str.length()) {
                    if(c ==' ')
                        last_space_pos = count;

                    /* check if the character will take us past the right hand side margin */
                    if( (cur_pos_x += char_width) > right_end) {
                        if(last_space_pos == start_index)              {
                            last_space_pos = count-2; // fake a break
                        }
                        temp_str = cur_str.substr(start_index, last_space_pos - start_index);
                        count = last_space_pos;//move the char indexer back to the last space position
                        start_index = last_space_pos;

                    	/* print the line */
                        TextOut(pdc,left_start,cur_pos_y,temp_str.c_str(), temp_str.length());
                    }
                    else // No we won't go past RHS margin */
                    {
                        continue;              
                    }
                }
                else {

                    temp_str = cur_str.substr(start_index, (count)-start_index);
                    /* print */
                    TextOut(pdc,left_start,cur_pos_y,temp_str.c_str(), temp_str.length());

                    start_index = count;
                }

                /* calculate the next print position - start new page if req'd */
                cur_pos_x = left_start;
                cur_pos_y += line_height;
                if (cur_pos_y > bottom_end) {
                    EndPage(pdc);
                    cur_pos_y = top_start+line_height;
                    StartPage(pdc);
                }
            }  
        }
    
        EndDoc(pdc);
        p.lines.clear();

        /* cleanup */
        SelectObject(pdc, hOldFont);
        DeleteObject(hFont);
        DeleteDC(pdc);
        return p;
    }
   
    pstream& pstream:: set_margin_l(pstream &p, const int margin) {
        p.setLeftMargin(margin);
        return p;
    }  
     
     pstream& pstream:: set_margin_r(pstream &p, const int margin) {
        p.setRightMargin(margin);
        return p;
    }

    pstream& pstream:: set_margin_t(pstream &p, const int margin) {
        p.setTopMargin(margin);
        return p;
    }

    pstream& pstream:: set_margin_b(pstream &p, const int margin) {
        p.setBottomMargin(margin);
        return p;
    }  
        
    std::vector< std::string>  pstream::get_printer_names() {

        LPBYTE buff = nullptr;
        DWORD buff_size = 0;
        DWORD bytes_needed;
        DWORD num_printers = 0;
        PRINTER_INFO_4 *pInfo;
        std::vector<std::string> names;

        if ( EnumPrinters(PRINTER_ENUM_LOCAL, nullptr,4,
                            buff,0,&bytes_needed,
                            &num_printers) == FALSE  && bytes_needed > 0)
        {
            buff = new byte[bytes_needed];
            EnumPrinters(PRINTER_ENUM_LOCAL, nullptr,4,buff,bytes_needed,&bytes_needed,&num_printers);

        }

        pInfo = reinterpret_cast<PRINTER_INFO_4*>(buff);
        for (int count =0; count < num_printers; count ++) {
            names.emplace_back(pInfo->pPrinterName);
            ++pInfo;          
        }
   
        delete [] buff;

        return names;
    }
}