#include "stdafx.h"
#include "helper_funcs.h"
#include "image_print.h"

#include "prn_stream.h"
#include "share/litePDF.h"
#include <comutil.h>

#pragma comment(lib,"comsuppw.lib")

using namespace litePDF;

std::string bstr_to_u32(const BSTR source) {
    const auto wrapped_bstr = _bstr_t(source);
    const int length = wrapped_bstr.length();
    const auto char_array = new char[length];
    strcpy_s(char_array, length + 1, wrapped_bstr);

    return char_array;
}

void print_wia_img(const std::string& file_name, const std::string& printer_name) {
    HDC pDC = CreateDC("WINSPOOL", printer_name.c_str(), nullptr, nullptr);

    image_print image(file_name.c_str());
    image.print(pDC);

    DeleteDC(pDC);
}

void bmp_to_pdf(const std::string& image_name, const std::string& dir_n_pdfname)
try {
    TLitePDF lite_pdf;

    /* create a document */
    lite_pdf.CreateMemDocument();

    /* create a page */
    const HDC hDC = lite_pdf.AddPage(lite_pdf.MMToUnit(210), lite_pdf.MMToUnit(297),
        795, 1124, LitePDFDrawFlag_SubstituteFonts);

    const auto wia_bitmap = static_cast<HBITMAP>(LoadImage(nullptr, image_name.c_str(),
                                                           IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE));
    if (nullptr == wia_bitmap)
        DWORD last_error = GetLastError();


    HDC hBDC = CreateCompatibleDC(hDC);
    // Ќазначаем изображение в этот контекст, сохран€ем предыдущее изображение в hOldBitmap
    HGDIOBJ hOldBitmap = SelectObject(hBDC, wia_bitmap);

    BITMAP bInfo;
    GetObject(wia_bitmap, sizeof(bInfo), &bInfo);

    /*  опирование содержимого контекста изображени€ (hBDC) в целевой контекст(hDC) */
    BitBlt(hDC,                                 // ƒескриптор целевого контекста (куда рисуем)
        0, 0, bInfo.bmWidth, bInfo.bmHeight,    // ѕр€моугольна€ область целевого контекста
        hBDC,                                   // ƒескриптор исходного контекста (который рисуем)
        0, 0,                                   //  оординаты левого верхнего угла исходного контекста
        SRCCOPY);                               // “ип наложени€ (SRCCOPY - замена пикселей целевого на пиксели исходного контекстов)

	/* ¬осстанавливаем предыдущее изображение в созданный контекст */
    SelectObject(hBDC, hOldBitmap);

    DeleteDC(hBDC);

    lite_pdf.FinishPage(hDC);
    lite_pdf.SaveToFile(dir_n_pdfname.c_str());
    lite_pdf.Close();
}
catch (TLitePDFException& ex) {
    fprintf(stderr, "lite_pdf exception: %x: %s\n", ex.getCode(), ex.getMessage());
}


bool fill_cb_events(CComboBox* combo_box_devices, CComboBox* combo_box_events, std::vector<act_event>* act_events) {
    for (std::vector<act_event>::iterator it = act_events->begin(); it != act_events->end(); ++it) {
        if (combo_box_devices->GetCount()-1 != it->index_of_device)
            combo_box_devices->AddString(it->device_name.c_str());
        else
        {
            combo_box_devices->SetItemData(it->index_of_device, reinterpret_cast<DWORD>(it->device_name.c_str()));            
        }
        if (0 == it->index_of_device)
            combo_box_events->AddString(it->btn_event_name.c_str());
    }
    return true;
}

bool load_printers(CComboBox* combo_box_printers) {
    std::vector<std::string> printer_names = pst::pstream::get_printer_names();
    for (auto it = printer_names.begin(); it != printer_names.end(); ++it) {
        combo_box_printers->AddString(it->c_str());
    }

    return true;
}

static std::string get_config_file_path(const int config_file_type) {
    CHAR current_path[MAX_PATH];

    const char* fn_ini = "";
    if (config_file_type == permanent_config)
        fn_ini = R"(\rus1scan.ini)";
    else
		if (config_file_type == temporary_config)
			fn_ini = R"(\rus1scan.inf)";

    DWORD res = GetModuleFileName(nullptr, current_path, MAX_PATH);

    /* remove filename from the path */
    PathRemoveFileSpec(current_path);
    strcat(current_path, fn_ini);

    /* Use MyDocuments in case of file not found */
    if (!std::filesystem::exists(std::string(current_path)) && config_file_type != temporary_config)
    {
	    CHAR my_documents[MAX_PATH];
	    HRESULT result = SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, my_documents);
        strcat(my_documents, fn_ini);
        return std::string(my_documents);
    }
    return std::string(current_path);
}

bool save_action_to_file(const std::string& action_to_save) {

	const std::string& action_to_launch = action_to_save;
    std::string action_for_ini = action_to_launch;

	/* removing leading and trailing spaces */
    action_for_ini = std::regex_replace(action_for_ini, std::regex("^ +| +$"), "$1");

    std::ofstream of;
    of.open(get_config_file_path(temporary_config), std::ofstream::out | std::ofstream::app);

    of << action_for_ini << "\n";

    of.flush();
    of.close();

    try {
        std::filesystem::remove(get_config_file_path(permanent_config));
        std::filesystem::rename(get_config_file_path(temporary_config),
            get_config_file_path(permanent_config));
    }
    catch (std::filesystem::filesystem_error& e) {
        std::cout << e.what() << '\n';
    }
    return true;
}

static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv_utf8_utf32;
/* convert a BSTR to a std::string */
static std::string& bstr_to_std_string(const BSTR bstr, std::string& dst) {
    if (!bstr) {
        dst.clear();
        return dst;
    }

    /* request content length in single-chars through a terminating
     * nullchar in the BSTR. note: BSTR's support imbedded nullchars,
     *  so this will only convert through the first nullchar
     */
    if (const int res = WideCharToMultiByte(CP_UTF8, 0, bstr, -1, nullptr, 0, nullptr, nullptr); res > 0) {
        dst.resize(res);
        WideCharToMultiByte(CP_UTF8, 0, bstr, -1, dst.data(), res, nullptr, nullptr);
    }
    else {
    	/* no content. clear target */
        dst.clear();
    }

    return dst;
}

std::string bstr_to_std_string(const BSTR bstr) {
    std::string str;
    bstr_to_std_string(bstr, str);
    return str;
}
