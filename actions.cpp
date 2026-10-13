// actions.cpp: действия над отсканированными изображениями (см. actions.h).

#include "stdafx.h"
#include "actions.h"
#include "BitmapUtil.h"
#include "strutil.h"
#pragma warning(push)
#pragma warning(disable : 4828)     // В комментариях litePDF.h есть символы не в UTF-8
#include "share\litePDF.h"
#pragma warning(pop)

#include <fstream>
#include <cmath>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace
{
    // Представление страницы BMP в памяти, пригодное для StretchDIBits.
    struct DibView
    {
        const BITMAPINFO* info = nullptr;   // Заголовок DIB (+ палитра)
        const void*       bits = nullptr;   // Пиксельные данные
        LONG              width = 0;        // Ширина в пикселях
        LONG              height = 0;       // Высота в пикселях (положительная)
        double            dpiX = 0;         // Разрешение по X (0 - неизвестно)
        double            dpiY = 0;         // Разрешение по Y (0 - неизвестно)
    };

    // Проверяет .bmp-файл в памяти и заполняет DibView.
    //   image - страница (полный .bmp с BITMAPFILEHEADER);
    //   view  - [out] указатели на заголовок и пиксели, размеры и разрешение.
    // Возвращает false, если данные не являются корректным BMP (или это старый OS/2-формат).
    bool GetDibView(const ScannedImage& image, DibView& view)
    {
        const size_t size = image.bmp.size();
        if (size < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
        {
            return false;
        }

        const BYTE* data = image.bmp.data();
        const BITMAPFILEHEADER* fileHeader = reinterpret_cast<const BITMAPFILEHEADER*>(data);
        const BITMAPINFOHEADER* infoHeader = reinterpret_cast<const BITMAPINFOHEADER*>(data + sizeof(BITMAPFILEHEADER));

        if (fileHeader->bfType != MAKEWORD('B', 'M') || infoHeader->biSize < sizeof(BITMAPINFOHEADER) ||
            fileHeader->bfOffBits >= size || infoHeader->biWidth <= 0 || infoHeader->biHeight == 0)
        {
            return false;
        }

        const LONG width = infoHeader->biWidth;
        const LONG height = std::labs(infoHeader->biHeight);

        if (infoHeader->biCompression == BI_RGB || infoHeader->biCompression == BI_BITFIELDS)
        {
            const unsigned long long needed =
                static_cast<unsigned long long>(BitmapUtil::GetBitmapLineWidthInBytes(width, infoHeader->biBitCount)) * height;
            if (fileHeader->bfOffBits + needed > size)
            {
                return false;
            }
        }

        view.info = reinterpret_cast<const BITMAPINFO*>(infoHeader);
        view.bits = data + fileHeader->bfOffBits;
        view.width = width;
        view.height = height;
        view.dpiX = infoHeader->biXPelsPerMeter > 0 ? infoHeader->biXPelsPerMeter * 0.0254 : 0;
        view.dpiY = infoHeader->biYPelsPerMeter > 0 ? infoHeader->biYPelsPerMeter * 0.0254 : 0;
        if (view.dpiX <= 0) view.dpiX = view.dpiY;
        if (view.dpiY <= 0) view.dpiY = view.dpiX;
        return true;
    }

    // Раскрывает шаблон папки (коды даты и переменные окружения) и создаёт папку.
    //   pattern - шаблон из конфига (пусто - папка по умолчанию C:\Scans\%d.%m.%Y\);
    //   now     - время события для подстановки даты;
    //   folder  - [out] готовый путь без завершающей "\";
    //   error   - [out] текст ошибки.
    // Возвращает true, если папка существует или создана.
    bool PrepareFolder(const std::wstring& pattern, const SYSTEMTIME& now, std::wstring& folder, std::wstring& error)
    {
        std::wstring expanded = strutil::ExpandDatePattern(pattern.empty() ? kDefaultScanFolder : pattern, now);

        wchar_t envExpanded[4096] = {};
        if (ExpandEnvironmentStringsW(expanded.c_str(), envExpanded, static_cast<DWORD>(COUNTOF(envExpanded))) > 0)
        {
            expanded = envExpanded;
        }

        expanded = strutil::Trim(expanded);
        while (expanded.size() > 3 && (expanded.back() == L'\\' || expanded.back() == L'/'))
        {
            expanded.pop_back();
        }
        if (expanded.empty())
        {
            error = L"не задана папка для сохранения";
            return false;
        }

        std::error_code ec;
        std::filesystem::create_directories(expanded, ec);
        if (!std::filesystem::is_directory(expanded, ec))
        {
            error = L"не удалось создать папку " + expanded;
            return false;
        }
        folder = expanded;
        return true;
    }

    // Возвращает путь %TEMP%\rus1scan (создаёт папку при необходимости) - для режима
    // "запустить программу", если папка сохранения не задана.
    std::wstring GetTempScanFolder()
    {
        wchar_t temp[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, temp);
        std::wstring folder = temp;
        if (!folder.empty() && folder.back() == L'\\')
        {
            folder.pop_back();
        }
        folder += L"\\rus1scan";
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        return folder;
    }

    // Формирует базовое имя файла: <префикс><ГГГГММДД_ЧЧММСС>.
    //   prefix - префикс из конфига (недопустимые символы заменяются на "_");
    //   now    - время сканирования.
    std::wstring MakeBaseName(const std::wstring& prefix, const SYSTEMTIME& now)
    {
        wchar_t stamp[32];
        swprintf_s(stamp, L"%04u%02u%02u_%02u%02u%02u", now.wYear, now.wMonth, now.wDay,
                   now.wHour, now.wMinute, now.wSecond);
        return strutil::SanitizeFileName(prefix) + stamp;
    }

    // Подбирает несуществующее имя файла: folder\base.ext, затем folder\base_(2).ext и т.д.
    //   folder    - папка;
    //   base      - имя без расширения;
    //   extension - расширение с точкой (".gif", ".pdf").
    std::wstring MakeUniquePath(const std::wstring& folder, const std::wstring& base, const std::wstring& extension)
    {
        std::wstring path = folder + L"\\" + base + extension;
        for (int n = 2; GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES && n < 10000; ++n)
        {
            path = folder + L"\\" + base + L"_(" + std::to_wstring(n) + L")" + extension;
        }
        return path;
    }

    // Записывает буфер в файл целиком (файл создаётся заново).
    //   path  - путь к файлу;
    //   data  - данные;
    //   error - [out] текст ошибки.
    bool WriteWholeFile(const std::wstring& path, const std::vector<BYTE>& data, std::wstring& error)
    {
        HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            error = L"не удалось создать файл " + path + L" (ошибка " + std::to_wstring(GetLastError()) + L")";
            return false;
        }
        DWORD written = 0;
        const BOOL ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, NULL);
        const DWORD code = GetLastError();
        CloseHandle(file);
        if (!ok || written != data.size())
        {
            error = L"ошибка записи файла " + path + L" (ошибка " + std::to_wstring(code) + L")";
            return false;
        }
        return true;
    }

    // Формирует имя файла страницы: <base> или <base>_pNN, если страниц несколько.
    //   base  - базовое имя (префикс + дата_время);
    //   index - номер страницы с 0;
    //   count - всего страниц.
    std::wstring PageFileName(const std::wstring& base, size_t index, size_t count)
    {
        if (count <= 1)
        {
            return base;
        }
        wchar_t page[16];
        swprintf_s(page, L"_p%02u", static_cast<unsigned>(index + 1));
        return base + page;
    }

    // Сеанс GDI+: инициализирует библиотеку в конструкторе и завершает в деструкторе.
    // GDI+ используется только для записи GIF.
    class GdiplusSession
    {
    public:
        // Запускает GDI+ (GdiplusStartup).
        GdiplusSession()
        {
            Gdiplus::GdiplusStartupInput input;
            m_ok = Gdiplus::GdiplusStartup(&m_token, &input, NULL) == Gdiplus::Ok;
        }

        // Завершает GDI+ (GdiplusShutdown), если запуск был успешным.
        ~GdiplusSession()
        {
            if (m_ok)
            {
                Gdiplus::GdiplusShutdown(m_token);
            }
        }

        GdiplusSession(const GdiplusSession&) = delete;
        GdiplusSession& operator=(const GdiplusSession&) = delete;

        // Возвращает true, если GDI+ запущен.
        bool Ok() const { return m_ok; }

    private:
        ULONG_PTR m_token = 0;
        bool      m_ok = false;
    };

    // Находит CLSID кодировщика GDI+ по MIME-типу.
    //   mimeType - MIME-тип, например L"image/gif";
    //   clsid    - [out] CLSID кодировщика.
    // Возвращает false, если кодировщик не найден.
    bool GetEncoderClsid(const wchar_t* mimeType, CLSID& clsid)
    {
        UINT count = 0;
        UINT size = 0;
        if (Gdiplus::GetImageEncodersSize(&count, &size) != Gdiplus::Ok || size == 0)
        {
            return false;
        }
        std::vector<BYTE> buffer(size);
        Gdiplus::ImageCodecInfo* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
        if (Gdiplus::GetImageEncoders(count, size, codecs) != Gdiplus::Ok)
        {
            return false;
        }
        for (UINT i = 0; i < count; ++i)
        {
            if (wcscmp(codecs[i].MimeType, mimeType) == 0)
            {
                clsid = codecs[i].Clsid;
                return true;
            }
        }
        return false;
    }

    // Записывает страницу (BMP в памяти) в файл GIF через GDI+. Изображения с палитрой
    // (ч/б, оттенки серого) сохраняются без потерь; полноцветные GDI+ сводит к палитре
    // из 256 цветов с дизерингом - это ограничение самого формата GIF.
    //   image - страница (полный .bmp в памяти);
    //   path  - путь создаваемого файла;
    //   clsid - CLSID кодировщика GIF;
    //   error - [out] текст ошибки.
    bool WriteGifFile(const ScannedImage& image, const std::wstring& path, const CLSID& clsid, std::wstring& error)
    {
        IStream* stream = SHCreateMemStream(image.bmp.data(), static_cast<UINT>(image.bmp.size()));
        if (stream == NULL)
        {
            error = L"недостаточно памяти для преобразования в GIF";
            return false;
        }

        Gdiplus::Status status;
        {
            Gdiplus::Bitmap bitmap(stream);
            status = bitmap.GetLastStatus();
            if (status == Gdiplus::Ok)
            {
                status = bitmap.Save(path.c_str(), &clsid, NULL);
            }
        }
        stream->Release();

        if (status != Gdiplus::Ok)
        {
            error = L"не удалось сохранить " + path + L" в GIF (код GDI+ " + std::to_wstring(status) + L")";
            return false;
        }
        return true;
    }

    // Преобразует сообщение litePDF (однобайтовая строка в кодовой странице системы) в UTF-16.
    //   text - сообщение исключения TLitePDFException.
    std::wstring AnsiToWide(const char* text)
    {
        if (text == nullptr || *text == '\0')
        {
            return std::wstring();
        }
        const int len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
        std::wstring result(static_cast<size_t>(len > 0 ? len : 1), L'\0');
        MultiByteToWideChar(CP_ACP, 0, text, -1, &result[0], len);
        result.resize(wcslen(result.c_str()));
        return result;
    }

    // Проверяет, работает ли процесс в сеансе 0 (служба) - там окна программ не видны пользователю.
    bool IsSessionZero()
    {
        DWORD session = 0;
        return ProcessIdToSessionId(GetCurrentProcessId(), &session) && session == 0;
    }
}

namespace actions
{
    // Перечисляет принтеры: локальные и сетевые подключения текущего пользователя.
    // Возвращает список имён (в службе, работающей от LocalSystem, сетевые принтеры
    // пользователей не видны).
    std::vector<std::wstring> EnumeratePrinters()
    {
        std::vector<std::wstring> names;
        const DWORD flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
        DWORD needed = 0;
        DWORD count = 0;

        EnumPrintersW(flags, NULL, 4, NULL, 0, &needed, &count);
        if (needed == 0)
        {
            return names;
        }

        std::vector<BYTE> buffer(needed);
        if (!EnumPrintersW(flags, NULL, 4, buffer.data(), needed, &needed, &count))
        {
            LOG_SAVE << "EnumPrinters failed, error " << GetLastError();
            return names;
        }

        const PRINTER_INFO_4W* info = reinterpret_cast<const PRINTER_INFO_4W*>(buffer.data());
        for (DWORD i = 0; i < count; ++i)
        {
            if (info[i].pPrinterName != NULL)
            {
                names.push_back(info[i].pPrinterName);
            }
        }
        return names;
    }

    // Возвращает имя принтера по умолчанию или пустую строку, если он не задан.
    std::wstring GetDefaultPrinterName()
    {
        DWORD size = 0;
        GetDefaultPrinterW(NULL, &size);
        if (size == 0)
        {
            return std::wstring();
        }
        std::wstring name(size, L'\0');
        if (!GetDefaultPrinterW(&name[0], &size))
        {
            return std::wstring();
        }
        name.resize(wcslen(name.c_str()));
        return name;
    }

    // Загружает .bmp-файл с диска в память (используется режимом /process для проверки
    // действий без сканера).
    //   path  - путь к .bmp-файлу;
    //   image - [out] содержимое файла;
    //   error - [out] текст ошибки.
    bool LoadBmpFile(const std::wstring& path, ScannedImage& image, std::wstring& error)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            error = L"не удалось открыть файл " + path;
            return false;
        }
        image.bmp.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());

        DibView view;
        if (!GetDibView(image, view))
        {
            error = L"файл " + path + L" не является поддерживаемым BMP";
            return false;
        }
        return true;
    }

    // Сохраняет страницы как отдельные .gif-файлы: <префикс><дата_время>[_pNN].gif.
    //   images - страницы;
    //   folder - готовая (существующая) директория;
    //   prefix - префикс имени файла;
    //   files  - [out] пути созданных файлов;
    //   error  - [out] текст ошибки.
    // Возвращает true, если сохранены все страницы.
    bool SaveImagesAsGif(const std::vector<ScannedImage>& images, const std::wstring& folder,
                         const std::wstring& prefix, std::vector<std::wstring>& files, std::wstring& error)
    {
        GdiplusSession gdiplus;
        CLSID gifClsid;
        if (!gdiplus.Ok() || !GetEncoderClsid(L"image/gif", gifClsid))
        {
            error = L"кодировщик GIF (GDI+) недоступен";
            return false;
        }

        SYSTEMTIME now;
        GetLocalTime(&now);
        const std::wstring base = MakeBaseName(prefix, now);

        for (size_t i = 0; i < images.size(); ++i)
        {
            const std::wstring path = MakeUniquePath(folder, PageFileName(base, i, images.size()), L".gif");
            if (!WriteGifFile(images[i], path, gifClsid, error))
            {
                return false;
            }
            LOG_SAVE << "Saved " << path;
            files.push_back(path);
        }
        return true;
    }

    // Сохраняет страницы как отдельные .bmp-файлы: <префикс><дата_время>[_pNN].bmp.
    // Данные записываются как есть, без преобразования.
    //   images - страницы;
    //   folder - готовая (существующая) директория;
    //   prefix - префикс имени файла;
    //   files  - [out] пути созданных файлов;
    //   error  - [out] текст ошибки.
    // Возвращает true, если сохранены все страницы.
    bool SaveImagesAsBmp(const std::vector<ScannedImage>& images, const std::wstring& folder,
                         const std::wstring& prefix, std::vector<std::wstring>& files, std::wstring& error)
    {
        SYSTEMTIME now;
        GetLocalTime(&now);
        const std::wstring base = MakeBaseName(prefix, now);

        for (size_t i = 0; i < images.size(); ++i)
        {
            const std::wstring path = MakeUniquePath(folder, PageFileName(base, i, images.size()), L".bmp");
            if (!WriteWholeFile(path, images[i].bmp, error))
            {
                return false;
            }
            LOG_SAVE << "Saved " << path;
            files.push_back(path);
        }
        return true;
    }

    // Сохраняет страницы отдельными файлами в формате из [common] filetype (gif или bmp).
    //   images - страницы;
    //   folder - готовая (существующая) директория;
    //   config - настройки (формат и префикс имени файла);
    //   files  - [out] пути созданных файлов;
    //   error  - [out] текст ошибки.
    // Возвращает true, если сохранены все страницы.
    bool SaveImagesAsPictures(const std::vector<ScannedImage>& images, const std::wstring& folder,
                              const Rus1Config& config, std::vector<std::wstring>& files, std::wstring& error)
    {
        if (config.fileType == ImageFileType::Bmp)
        {
            return SaveImagesAsBmp(images, folder, config.filePrefix, files, error);
        }
        return SaveImagesAsGif(images, folder, config.filePrefix, files, error);
    }

    // Сохраняет все страницы в один PDF (по странице PDF на страницу скана) через litePDF.dll.
    // Размер страницы PDF берётся из разрешения скана (скан A4 даёт страницу A4); если
    // разрешение неизвестно, изображение вписывается в страницу A4.
    //   images - страницы;
    //   folder - готовая (существующая) папка;
    //   prefix - префикс имени файла;
    //   file   - [out] путь созданного PDF;
    //   error  - [out] текст ошибки.
    // Возвращает true при успехе.
    bool SaveImagesAsPdf(const std::vector<ScannedImage>& images, const std::wstring& folder,
                         const std::wstring& prefix, std::wstring& file, std::wstring& error)
    {
        SYSTEMTIME now;
        GetLocalTime(&now);
        const std::wstring path = MakeUniquePath(folder, MakeBaseName(prefix, now), L".pdf");

        try
        {
            litePDF::TLitePDF pdf;
            pdf.SetUnit(litePDF::LitePDFUnit_1000th_mm);
            pdf.CreateMemDocument();

            int pages = 0;
            for (const auto& image : images)
            {
                DibView view;
                if (!GetDibView(image, view))
                {
                    LOG_SAVE << "PDF: page skipped - invalid BMP data";
                    continue;
                }

                double pageWidthMm;
                double pageHeightMm;
                unsigned int canvasWidth;
                unsigned int canvasHeight;

                if (view.dpiX > 0 && view.dpiY > 0)
                {
                    // Размер страницы = физический размер скана
                    pageWidthMm = view.width / view.dpiX * 25.4;
                    pageHeightMm = view.height / view.dpiY * 25.4;
                    canvasWidth = static_cast<unsigned int>(view.width);
                    canvasHeight = static_cast<unsigned int>(view.height);
                }
                else
                {
                    // Разрешение неизвестно - вписываем изображение в A4 с сохранением пропорций
                    pageWidthMm = 210.0;
                    pageHeightMm = 297.0;
                    const double fit = (std::min)(210.0 / view.width, 297.0 / view.height);  // мм на пиксель
                    canvasWidth = static_cast<unsigned int>(std::ceil(210.0 / fit));
                    canvasHeight = static_cast<unsigned int>(std::ceil(297.0 / fit));
                }

                HDC dc = pdf.AddPage(static_cast<unsigned int>(std::lround(pageWidthMm * 1000.0)),
                                     static_cast<unsigned int>(std::lround(pageHeightMm * 1000.0)),
                                     canvasWidth, canvasHeight,
                                     litePDF::LitePDFDrawFlag_CompressImagesWithJPEG);

                SetStretchBltMode(dc, COLORONCOLOR);
                const int lines = StretchDIBits(dc, 0, 0, view.width, view.height, 0, 0, view.width, view.height,
                                                view.bits, view.info, DIB_RGB_COLORS, SRCCOPY);
                if (lines == 0 || lines == GDI_ERROR)
                {
                    LOG_SAVE << "PDF: StretchDIBits failed, error " << GetLastError();
                }

                pdf.FinishPage(dc);
                ++pages;
            }

            if (pages == 0)
            {
                pdf.Close();
                error = L"нет страниц для PDF";
                return false;
            }

            pdf.SaveToFileW(path.c_str());
            pdf.Close();
        }
        catch (litePDF::TLitePDFException& ex)
        {
            error = L"ошибка litePDF " + std::to_wstring(ex.getCode()) + L": " + AnsiToWide(ex.getMessage());
            return false;
        }

        LOG_SAVE << "Saved " << path;
        file = path;
        return true;
    }

    // Печатает страницы одним заданием. Изображение печатается в физическом размере
    // (по разрешению скана) от левого верхнего угла области печати; если оно не
    // помещается или разрешение неизвестно - вписывается в страницу с сохранением пропорций.
    //   images  - страницы;
    //   printer - имя принтера; пусто - принтер по умолчанию;
    //   error   - [out] текст ошибки.
    // Возвращает true, если задание печати отправлено.
    bool PrintImages(const std::vector<ScannedImage>& images, const std::wstring& printer, std::wstring& error)
    {
        const std::wstring printerName = printer.empty() ? GetDefaultPrinterName() : printer;
        if (printerName.empty())
        {
            error = L"принтер не указан и принтер по умолчанию не задан";
            return false;
        }

        HDC dc = CreateDCW(L"WINSPOOL", printerName.c_str(), NULL, NULL);
        if (dc == NULL)
        {
            error = L"не удалось открыть принтер \"" + printerName + L"\" (ошибка " + std::to_wstring(GetLastError()) + L")";
            return false;
        }

        DOCINFOW docInfo = {};
        docInfo.cbSize = sizeof(docInfo);
        docInfo.lpszDocName = L"rus1scan";
        if (StartDocW(dc, &docInfo) <= 0)
        {
            error = L"StartDoc на принтере \"" + printerName + L"\" не удался (ошибка " + std::to_wstring(GetLastError()) + L")";
            DeleteDC(dc);
            return false;
        }

        const int pageWidth = GetDeviceCaps(dc, HORZRES);
        const int pageHeight = GetDeviceCaps(dc, VERTRES);
        const int printerDpiX = GetDeviceCaps(dc, LOGPIXELSX);
        const int printerDpiY = GetDeviceCaps(dc, LOGPIXELSY);

        int printed = 0;
        for (const auto& image : images)
        {
            DibView view;
            if (!GetDibView(image, view))
            {
                LOG_SAVE << "Print: page skipped - invalid BMP data";
                continue;
            }

            double targetWidth = view.width;
            double targetHeight = view.height;
            bool fit = true;
            if (view.dpiX > 0 && view.dpiY > 0)
            {
                targetWidth = view.width * printerDpiX / view.dpiX;
                targetHeight = view.height * printerDpiY / view.dpiY;
                fit = targetWidth > pageWidth || targetHeight > pageHeight;
            }
            if (fit)
            {
                const double scale = (std::min)(static_cast<double>(pageWidth) / view.width,
                                                static_cast<double>(pageHeight) / view.height);
                targetWidth = view.width * scale;
                targetHeight = view.height * scale;
            }

            StartPage(dc);
            SetStretchBltMode(dc, HALFTONE);
            SetBrushOrgEx(dc, 0, 0, NULL);
            const int lines = StretchDIBits(dc, 0, 0, static_cast<int>(targetWidth), static_cast<int>(targetHeight),
                                            0, 0, view.width, view.height, view.bits, view.info,
                                            DIB_RGB_COLORS, SRCCOPY);
            if (lines == 0 || lines == GDI_ERROR)
            {
                LOG_SAVE << "Print: StretchDIBits failed, error " << GetLastError();
            }
            EndPage(dc);
            ++printed;
        }

        EndDoc(dc);
        DeleteDC(dc);

        if (printed == 0)
        {
            error = L"нет страниц для печати";
            return false;
        }
        LOG_SAVE << "Printed " << printed << " page(s) on '" << printerName << "'";
        return true;
    }

    // Запускает программу для каждого файла. В командной строке "%1" заменяется путём к
    // файлу; если "%1" нет, путь в кавычках добавляется в конец.
    //   commandTemplate - командная строка из конфига, например: mspaint.exe "%1";
    //   files           - файлы, которые нужно передать программе;
    //   error           - [out] текст ошибки.
    // Возвращает true, если все процессы запущены. Внимание: из службы (сеанс 0) окна
    // запущенных программ пользователю не видны.
    bool StartProgram(const std::wstring& commandTemplate, const std::vector<std::wstring>& files, std::wstring& error)
    {
        if (strutil::Trim(commandTemplate).empty())
        {
            error = L"не указана программа для запуска";
            return false;
        }
        if (IsSessionZero())
        {
            LOG_SAVE << "Warning: running in session 0 (service) - started program windows are not visible to users";
        }

        for (const auto& file : files)
        {
            std::wstring command = commandTemplate;
            const size_t placeholder = command.find(L"%1");
            if (placeholder != std::wstring::npos)
            {
                for (size_t pos = placeholder; pos != std::wstring::npos; pos = command.find(L"%1", pos + file.size()))
                {
                    command.replace(pos, 2, file);
                }
            }
            else
            {
                command += L" \"" + file + L"\"";
            }

            STARTUPINFOW si = {};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi = {};
            std::vector<wchar_t> buffer(command.begin(), command.end());
            buffer.push_back(L'\0');

            if (!CreateProcessW(NULL, buffer.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                error = L"не удалось запустить: " + command + L" (ошибка " + std::to_wstring(GetLastError()) + L")";
                return false;
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            LOG_SAVE << "Started: " << command;
        }
        return true;
    }

    // Выполняет действие привязки над уже полученными страницами.
    //   binding - привязка (действие и его параметры);
    //   config  - общие настройки (префикс имени файла);
    //   images  - страницы.
    // Возвращает true при успехе; ошибки записываются в журнал.
    bool RunActionOnImages(const EventBinding& binding, const Rus1Config& config,
                           const std::vector<ScannedImage>& images)
    {
        SYSTEMTIME now;
        GetLocalTime(&now);
        std::wstring error;
        bool ok = false;

        switch (binding.action)
        {
        case Rus1Action::Nothing:
            LOG_SAVE << "Action 'nothing' - images ignored";
            return true;

        case Rus1Action::Save:
        {
            std::wstring folder;
            if (!PrepareFolder(binding.folder, now, folder, error))
            {
                break;
            }
            if (binding.pdf)
            {
                std::wstring file;
                ok = SaveImagesAsPdf(images, folder, config.filePrefix, file, error);
            }
            else
            {
                std::vector<std::wstring> files;
                ok = SaveImagesAsPictures(images, folder, config, files, error);
            }
            break;
        }

        case Rus1Action::Print:
            ok = PrintImages(images, binding.printer, error);
            break;

        case Rus1Action::Start:
        {
            std::wstring folder;
            if (strutil::Trim(binding.folder).empty())
            {
                folder = GetTempScanFolder();
            }
            else if (!PrepareFolder(binding.folder, now, folder, error))
            {
                break;
            }

            std::vector<std::wstring> files;
            if (binding.pdf)
            {
                std::wstring file;
                ok = SaveImagesAsPdf(images, folder, config.filePrefix, file, error);
                if (ok)
                {
                    files.push_back(file);
                }
            }
            else
            {
                ok = SaveImagesAsPictures(images, folder, config, files, error);
            }
            if (ok)
            {
                ok = StartProgram(binding.program, files, error);
            }
            break;
        }
        }

        if (!ok)
        {
            LOG_SAVE << "Action '" << ActionToString(binding.action) << "' failed: " << error;
        }
        return ok;
    }

    // Полный цикл обработки события: сканирование с устройства и выполнение действия.
    //   binding  - привязка, найденная для события;
    //   config   - общие настройки (свойства [wia], префикс имени файла);
    //   deviceId - устройство, от которого пришло событие (может отличаться от binding.deviceId,
    //              если привязка задана для "любого устройства");
    //   itemName - полное имя элемента из события (может быть пустым).
    // Возвращает true при успехе; ошибки записываются в журнал.
    bool ExecuteBinding(const EventBinding& binding, const Rus1Config& config,
                        const std::wstring& deviceId, const std::wstring& itemName)
    {
        if (binding.action == Rus1Action::Nothing)
        {
            LOG_SAVE << "Binding action is 'nothing' - no scan";
            return true;
        }

        LOG_SAVE << "Scanning from " << deviceId << " for action '" << ActionToString(binding.action) << "'";
        std::vector<ScannedImage> images;
        const HRESULT hr = CRus1scan::AcquireImages(deviceId, itemName, config.wiaProps, images);
        if (FAILED(hr))
        {
            LOG_SAVE << "Scan failed: " << rLogger::HrText(hr);
            return false;
        }
        return RunActionOnImages(binding, config, images);
    }
}
