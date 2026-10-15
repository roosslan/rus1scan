// tests.cpp: тесты rus1scan (консольная программа без внешних библиотек).
//
// Запуск:  rus1scan_tests.exe [--exe <путь к rus1scan.exe>]
//   без параметров - модульные тесты: TOML, конфиг, строки, имена WIA, сохранение GIF/BMP/PDF;
//   --exe          - дополнительно дымовой тест готового rus1scan.exe в режиме /process.
// Код завершения: 0 - все тесты прошли, 1 - есть ошибки.

#include "stdafx.h"

#include "actions.h"
#include "config.h"
#include "strutil.h"
#include "toml_lite.h"
#include "wia_names.h"

#include <cstdio>
#include <fstream>

namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // Засчитывает проверку: при неудаче печатает место и выражение.
    //   ok   - результат проверки;
    //   expr - текст проверяемого выражения;
    //   file - исходный файл;
    //   line - строка.
    void Check(bool ok, const char* expr, const char* file, int line)
    {
        if (ok)
        {
            ++g_passed;
            return;
        }
        ++g_failed;
        std::printf("  FAILED %s:%d: %s\n", file, line, expr);
    }

#define CHECK(expr) Check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

    // Создаёт пустую временную директорию для теста и возвращает путь к ней.
    //   name - имя поддиректории.
    std::wstring MakeTempDir(const std::wstring& name)
    {
        wchar_t temp[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, temp);
        const std::filesystem::path dir = std::filesystem::path(temp) / L"rus1scan_tests" /
                                          (name + L"_" + std::to_wstring(GetCurrentProcessId()));
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        return dir.wstring();
    }

    // Записывает строку UTF-8 в файл.
    //   path - путь к файлу;
    //   text - содержимое.
    void WriteTextFile(const std::wstring& path, const std::string& text)
    {
        std::ofstream out(path, std::ios::binary);
        out << text;
    }

    // Читает первые байты файла (для проверки сигнатуры формата).
    //   path  - путь к файлу;
    //   count - сколько байт прочитать.
    std::string ReadHead(const std::wstring& path, size_t count)
    {
        std::ifstream in(path, std::ios::binary);
        std::string data(count, '\0');
        in.read(&data[0], static_cast<std::streamsize>(count));
        data.resize(static_cast<size_t>(in.gcount()));
        return data;
    }

    // Создаёт тестовую страницу: 24-битный BMP width x height, 100 DPI, градиент.
    //   width  - ширина в пикселях;
    //   height - высота в пикселях.
    ScannedImage MakeTestImage(int width, int height)
    {
        const int row = (width * 3 + 3) & ~3;
        const DWORD pixels = static_cast<DWORD>(row * height);

        BITMAPFILEHEADER fileHeader = {};
        fileHeader.bfType = MAKEWORD('B', 'M');
        fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        fileHeader.bfSize = fileHeader.bfOffBits + pixels;

        BITMAPINFOHEADER info = {};
        info.biSize = sizeof(info);
        info.biWidth = width;
        info.biHeight = height;
        info.biPlanes = 1;
        info.biBitCount = 24;
        info.biCompression = BI_RGB;
        info.biSizeImage = pixels;
        info.biXPelsPerMeter = info.biYPelsPerMeter = 3937;     // 100 DPI

        ScannedImage image;
        image.bmp.resize(fileHeader.bfSize);
        memcpy(image.bmp.data(), &fileHeader, sizeof(fileHeader));
        memcpy(image.bmp.data() + sizeof(fileHeader), &info, sizeof(info));
        BYTE* data = image.bmp.data() + fileHeader.bfOffBits;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                BYTE* p = data + y * row + x * 3;
                p[0] = static_cast<BYTE>(x * 255 / width);
                p[1] = static_cast<BYTE>(y * 255 / height);
                p[2] = 180;
            }
        }
        return image;
    }

    // Возвращает все файлы в директории (рекурсивно).
    //   dir - директория.
    std::vector<std::wstring> ListFiles(const std::wstring& dir)
    {
        std::vector<std::wstring> files;
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec))
        {
            if (entry.is_regular_file())
            {
                files.push_back(entry.path().wstring());
            }
        }
        return files;
    }

    // Возвращает true, если путь оканчивается на указанное расширение (без учёта регистра).
    //   path      - путь к файлу;
    //   extension - расширение с точкой.
    bool HasExtension(const std::wstring& path, const wchar_t* extension)
    {
        return strutil::EqualsNoCase(std::filesystem::path(path).extension().wstring(), extension);
    }

    // ---------------------------------------------------------------------------
    // Тесты

    // Разбор TOML: таблицы, массивы таблиц, строки обоих видов, числа, логические значения.
    void TestTomlParse()
    {
        const std::string text =
            "\xEF\xBB\xBF# комментарий\r\n"
            "[common]\r\n"
            "name = \"a\\tb\\u0416\"   # хвостовой комментарий\r\n"
            "path = 'C:\\Scans\\%d\\'\r\n"
            "num = -1_000\r\n"
            "flag = true\r\n"
            "\"Quoted Key\" = 5\r\n"
            "[[item]]\r\n"
            "x = 1\r\n"
            "[[item]]\r\n"
            "x = 2\r\n";

        tomllite::Document doc;
        std::string error;
        CHECK(tomllite::Parse(text, doc, error));
        const tomllite::Table* common = doc.FindTable("common");
        CHECK(common != nullptr);
        if (common == nullptr)
        {
            return;
        }
        CHECK(common->Find("name")->str == "a\tb\xD0\x96");
        CHECK(common->Find("path")->str == "C:\\Scans\\%d\\");
        CHECK(common->Find("num")->integer == -1000);
        CHECK(common->Find("flag")->boolean);
        CHECK(common->Find("Quoted Key")->integer == 5);
        const auto items = doc.FindArray("item");
        CHECK(items.size() == 2);
        CHECK(items.size() == 2 && items[1]->Find("x")->integer == 2);
    }

    // Ошибки TOML сообщаются с номером строки.
    void TestTomlErrors()
    {
        const char* bad[] =
        {
            "[a]\nx = \n",              // нет значения
            "[a]\nx = 1.5\n",           // дробное число
            "[a]\nx = 'abc\n",          // не закрыта кавычка
            "[a]\nx = 1\nx = 2\n",      // повтор ключа
            "[a]\n[a]\n",               // повтор таблицы
            "[a]\nx = [1, 2]\n",        // массив
            "[a]\nx = \"\\q\"\n",       // неизвестная escape-последовательность
        };
        for (const char* text : bad)
        {
            tomllite::Document doc;
            std::string error;
            CHECK(!tomllite::Parse(text, doc, error));
            CHECK(error.find("строка") == 0);
        }
    }

    // QuoteString выбирает литеральную форму для путей и экранирует остальное;
    // результат разбирается обратно в исходную строку.
    void TestTomlQuote()
    {
        CHECK(tomllite::QuoteString("C:\\Scans\\") == "'C:\\Scans\\'");
        CHECK(tomllite::QuoteKey("WIA_IPS_XRES") == "WIA_IPS_XRES");
        CHECK(tomllite::QuoteKey("Horizontal Resolution") == "'Horizontal Resolution'");

        const std::string samples[] = { "it's", "line\nbreak", "tab\there", "quote\"s", "Сканирование_" };
        for (const auto& sample : samples)
        {
            tomllite::Document doc;
            std::string error;
            CHECK(tomllite::Parse("[t]\nv = " + tomllite::QuoteString(sample) + "\n", doc, error));
            const tomllite::Table* table = doc.FindTable("t");
            CHECK(table != nullptr && table->Find("v") != nullptr && table->Find("v")->str == sample);
        }
    }

    // Строковые функции: шаблон даты, GUID, имена файлов, сравнение без учёта регистра.
    void TestStrutil()
    {
        SYSTEMTIME time = {};
        time.wYear = 2026;
        time.wMonth = 3;
        time.wDay = 7;
        time.wHour = 9;
        time.wMinute = 5;
        time.wSecond = 1;
        CHECK(strutil::ExpandDatePattern(L"C:\\Scans\\%d.%m.%Y\\", time) == L"C:\\Scans\\07.03.2026\\");
        CHECK(strutil::ExpandDatePattern(L"%y-%H%M%S %% %Q", time) == L"26-090501 % %Q");

        GUID guid;
        CHECK(strutil::StringToGuid(L"a6c5a715-8c6e-11d2-977a-0000f87a926f", guid));
        CHECK(strutil::GuidToString(guid) == L"{A6C5A715-8C6E-11D2-977A-0000F87A926F}");
        CHECK(!strutil::StringToGuid(L"не GUID", guid));

        CHECK(strutil::SanitizeFileName(L"a:b*c?") == L"a_b_c_");
        CHECK(strutil::Trim(L"  x y \t") == L"x y");
        CHECK(strutil::EqualsNoCase(L"Скан", L"СКАН"));
        CHECK(strutil::Utf8ToWide(strutil::WideToUtf8(L"Тест ✓")) == L"Тест ✓");
    }

    // Имена свойств и констант WIA для секции [wia].
    void TestWiaNames()
    {
        unsigned long id = 0;
        CHECK(wianames::FindPropertyId(L"WIA_IPS_XRES", id) && id == WIA_IPS_XRES);
        CHECK(wianames::FindPropertyId(L"wia_ips_yres", id) && id == WIA_IPS_YRES);
        CHECK(wianames::FindPropertyId(L"Horizontal Resolution", id) && id == WIA_IPS_XRES);
        CHECK(wianames::FindPropertyId(L"6147", id) && id == 6147);
        CHECK(!wianames::FindPropertyId(L"WIA_IPS_XRESS", id));
        CHECK(wianames::PropertyName(WIA_DPS_DOCUMENT_HANDLING_SELECT) == L"WIA_DPS_DOCUMENT_HANDLING_SELECT");

        long value = 0;
        CHECK(wianames::ParseIntegerValue(L"FEEDER|DUPLEX", value) && value == (FEEDER | DUPLEX));
        CHECK(wianames::ParseIntegerValue(L"WIA_INTENT_IMAGE_TYPE_COLOR", value) && value == WIA_INTENT_IMAGE_TYPE_COLOR);
        CHECK(wianames::ParseIntegerValue(L"0x10", value) && value == 16);
        CHECK(wianames::ParseIntegerValue(L" 300 ", value) && value == 300);
        CHECK(!wianames::ParseIntegerValue(L"FEEDR", value));

        GUID guid;
        CHECK(wianames::ParseFormatGuid(L"WiaImgFmt_BMP", guid) && IsEqualGUID(guid, WiaImgFmt_BMP));
        CHECK(wianames::ParseFormatGuid(L"jpeg", guid) && IsEqualGUID(guid, WiaImgFmt_JPEG));
    }

    // Конфиг: чтение всех секций, сохранение и повторное чтение без потерь.
    void TestConfigRoundTrip()
    {
        const std::wstring dir = MakeTempDir(L"config");
        const std::wstring path = dir + L"\\rus1scan.toml";
        WriteTextFile(path,
            "[common]\n"
            "filePrefix = 'Скан_'\n"
            "filetype = 'BMP'\n"
            "[gui]\n"
            "showDisconnectedDevices = true\n"
            "[wia]\n"
            "WIA_IPS_XRES = 300\n"
            "'Vertical Resolution' = 300\n"
            "WIA_DPS_DOCUMENT_HANDLING_SELECT = 'FEEDER|DUPLEX'\n"
            "[[binding]]\n"
            "deviceId = 'DEV\\0001'\n"
            "eventGuid = 'a6c5a715-8c6e-11d2-977a-0000f87a926f'\n"
            "eventFlags = 3\n"
            "action = 'Save'\n"
            "folder = 'C:\\Scans\\%d.%m.%Y\\'\n"
            "pdf = true\n"
            "[[binding]]\n"
            "eventGuid = '{9D095B89-37D6-4877-AFED-62A297DC6DBE}'\n"
            "action = 'print'\n"
            "printer = 'HP'\n");

        Rus1Config config;
        std::wstring error;
        CHECK(LoadConfig(path, config, error));
        CHECK(config.filePrefix == L"Скан_");
        CHECK(config.fileType == ImageFileType::Bmp);
        CHECK(config.showDisconnectedDevices);
        CHECK(config.wiaProps.size() == 3);
        CHECK(config.wiaProps.size() == 3 && config.wiaProps[1].propId == WIA_IPS_YRES);
        CHECK(config.bindings.size() == 2);
        if (config.bindings.size() != 2)
        {
            return;
        }
        CHECK(config.bindings[0].eventGuid == L"{A6C5A715-8C6E-11D2-977A-0000F87A926F}");
        CHECK(config.bindings[0].action == Rus1Action::Save && config.bindings[0].pdf);
        CHECK(config.bindings[0].eventFlags == 3);

        CHECK(SaveConfig(path, config, error));
        Rus1Config reloaded;
        CHECK(LoadConfig(path, reloaded, error));
        CHECK(reloaded.filePrefix == config.filePrefix);
        CHECK(reloaded.fileType == config.fileType);
        CHECK(reloaded.showDisconnectedDevices == config.showDisconnectedDevices);
        CHECK(reloaded.wiaProps.size() == config.wiaProps.size());
        CHECK(reloaded.bindings.size() == config.bindings.size());
        CHECK(reloaded.bindings.size() == 2 && reloaded.bindings[1].printer == L"HP");
        CHECK(reloaded.bindings.size() == 2 && reloaded.bindings[0].folder == L"C:\\Scans\\%d.%m.%Y\\");

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // Конфиг: значения по умолчанию, отсутствие файла, ошибки с номером строки.
    void TestConfigErrors()
    {
        const std::wstring dir = MakeTempDir(L"config_errors");
        const std::wstring path = dir + L"\\rus1scan.toml";

        Rus1Config config;
        std::wstring error;
        CHECK(LoadConfig(dir + L"\\missing.toml", config, error));
        CHECK(config.fileType == ImageFileType::Gif);
        CHECK(config.bindings.empty());

        const char* bad[] =
        {
            "[common]\nfiletype = 'png'\n",
            "[wia]\nWIA_IPS_XRESS = 1\n",
            "[wia]\nWIA_IPS_XRES = 1\n6147 = 2\n",
            "[[binding]]\naction = 'save'\n",
            "[[binding]]\neventGuid = 'x'\n",
            "[[binding]]\neventGuid = '{A6C5A715-8C6E-11D2-977A-0000F87A926F}'\naction = 'fly'\n",
        };
        for (const char* text : bad)
        {
            WriteTextFile(path, text);
            CHECK(!LoadConfig(path, config, error));
            CHECK(error.find(L"строка") == 0);
        }

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // Поиск привязки: точное устройство важнее "любого устройства".
    void TestFindBinding()
    {
        Rus1Config config;
        EventBinding any;
        any.eventGuid = L"{A6C5A715-8C6E-11D2-977A-0000F87A926F}";
        any.action = Rus1Action::Print;
        EventBinding exact = any;
        exact.deviceId = L"DEV\\0001";
        exact.action = Rus1Action::Save;
        config.bindings = { any, exact };

        const EventBinding* found = config.FindForEvent(L"dev\\0001", L"{a6c5a715-8c6e-11d2-977a-0000f87a926f}");
        CHECK(found != nullptr && found->action == Rus1Action::Save);
        found = config.FindForEvent(L"OTHER", any.eventGuid);
        CHECK(found != nullptr && found->action == Rus1Action::Print);
        CHECK(config.FindForEvent(L"DEV\\0001", L"{00000000-0000-0000-0000-000000000001}") == nullptr);
        CHECK(config.FindExact(L"", any.eventGuid) != nullptr);
    }

    // Сохранение страниц в GIF, BMP (по файлу на страницу) и PDF (один файл).
    void TestSaveFormats()
    {
        const std::wstring dir = MakeTempDir(L"save");
        const std::vector<ScannedImage> images = { MakeTestImage(64, 48), MakeTestImage(32, 32) };
        std::wstring error;

        std::vector<std::wstring> files;
        CHECK(actions::SaveImagesAsGif(images, dir, L"Тест_", files, error));
        CHECK(files.size() == 2);
        for (const auto& file : files)
        {
            CHECK(HasExtension(file, L".gif") && ReadHead(file, 6) == "GIF89a");
        }

        files.clear();
        CHECK(actions::SaveImagesAsBmp(images, dir, L"Тест_", files, error));
        CHECK(files.size() == 2);
        for (size_t i = 0; i < files.size(); ++i)
        {
            CHECK(HasExtension(files[i], L".bmp"));
            CHECK(std::filesystem::file_size(files[i]) == images[i].bmp.size());
        }

        std::wstring pdf;
        CHECK(actions::SaveImagesAsPdf(images, dir, L"Тест_", pdf, error));
        CHECK(HasExtension(pdf, L".pdf") && ReadHead(pdf, 5) == "%PDF-");

        // Повторное сохранение не затирает файлы, а получает уникальное имя
        files.clear();
        CHECK(actions::SaveImagesAsBmp({ images[0] }, dir, L"Тест_", files, error));
        CHECK(ListFiles(dir).size() == 6);

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // Действие "save": папка по шаблону даты создаётся, формат берётся из filetype.
    void TestRunActionSave()
    {
        const std::wstring dir = MakeTempDir(L"action");
        Rus1Config config;
        config.filePrefix = L"A_";
        EventBinding binding;
        binding.action = Rus1Action::Save;
        binding.folder = dir + L"\\%Y\\";

        config.fileType = ImageFileType::Bmp;
        CHECK(actions::RunActionOnImages(binding, config, { MakeTestImage(16, 16) }));
        config.fileType = ImageFileType::Gif;
        CHECK(actions::RunActionOnImages(binding, config, { MakeTestImage(16, 16) }));
        binding.pdf = true;
        CHECK(actions::RunActionOnImages(binding, config, { MakeTestImage(16, 16) }));

        SYSTEMTIME now;
        GetLocalTime(&now);
        const std::wstring yearDir = dir + L"\\" + std::to_wstring(now.wYear);
        int bmp = 0, gif = 0, pdf = 0;
        for (const auto& file : ListFiles(yearDir))
        {
            bmp += HasExtension(file, L".bmp");
            gif += HasExtension(file, L".gif");
            pdf += HasExtension(file, L".pdf");
        }
        CHECK(bmp == 1 && gif == 1 && pdf == 1);

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // Запускает процесс и ждёт его завершения.
    //   commandLine - командная строка;
    //   exitCode    - [out] код завершения.
    // Возвращает false, если процесс не запустился или не завершился за 60 секунд
    // (тогда процесс принудительно завершается).
    bool RunProcess(const std::wstring& commandLine, DWORD& exitCode)
    {
        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        std::vector<wchar_t> buffer(commandLine.begin(), commandLine.end());
        buffer.push_back(L'\0');
        if (!CreateProcessW(NULL, buffer.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            return false;
        }
        const bool finished = WaitForSingleObject(pi.hProcess, 60000) == WAIT_OBJECT_0;
        if (!finished)
        {
            TerminateProcess(pi.hProcess, 1);     // Не оставляем зависший процесс
            WaitForSingleObject(pi.hProcess, 5000);
        }
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return finished;
    }

    // Дымовой тест готового rus1scan.exe: копия exe с litePDF.dll и конфигом во временной
    // директории, режим /process сохраняет BMP-файл в GIF; ошибка в конфиге даёт код 3,
    // неизвестное событие - код 4. Все запуски с /quiet - без окон с сообщениями.
    //   exePath - путь к собранному rus1scan.exe.
    void TestExeProcess(const std::wstring& exePath)
    {
        const std::wstring dir = MakeTempDir(L"exe");
        const std::filesystem::path source(exePath);
        std::error_code ec;
        std::filesystem::copy_file(source, dir + L"\\rus1scan.exe", ec);
        CHECK(!ec);
        std::filesystem::copy_file(source.parent_path() / L"litePDF.dll", dir + L"\\litePDF.dll", ec);
        CHECK(!ec);

        const ScannedImage image = MakeTestImage(40, 30);
        {
            std::ofstream out(dir + L"\\input.bmp", std::ios::binary);
            out.write(reinterpret_cast<const char*>(image.bmp.data()), static_cast<std::streamsize>(image.bmp.size()));
        }

        const std::string outDir = strutil::WideToUtf8(dir) + "\\out\\";
        WriteTextFile(dir + L"\\rus1scan.toml",
            "[common]\nfilePrefix = 'Скан_'\nfiletype = 'gif'\n"
            "[[binding]]\neventGuid = '{A6C5A715-8C6E-11D2-977A-0000F87A926F}'\naction = 'save'\n"
            "folder = '" + outDir + "'\n");

        // /quiet: без консоли rus1scan показывает ошибки окном MessageBox, а тесту окна не нужны
        const std::wstring exe = L"\"" + dir + L"\\rus1scan.exe\" /quiet /process \"" + dir + L"\\input.bmp\" ";
        DWORD exitCode = 0;
        CHECK(RunProcess(exe + L"{A6C5A715-8C6E-11D2-977A-0000F87A926F}", exitCode));
        CHECK(exitCode == 0);
        const auto files = ListFiles(dir + L"\\out");
        CHECK(files.size() == 1 && HasExtension(files[0], L".gif") && ReadHead(files[0], 6) == "GIF89a");

        CHECK(RunProcess(exe + L"{00000000-0000-0000-0000-000000000001}", exitCode));
        CHECK(exitCode == 4);

        WriteTextFile(dir + L"\\rus1scan.toml", "[common]\nfiletype = 'tiff'\n");
        CHECK(RunProcess(exe + L"{A6C5A715-8C6E-11D2-977A-0000F87A926F}", exitCode));
        CHECK(exitCode == 3);

        std::filesystem::remove_all(dir, ec);
    }
}

// Точка входа: запускает все тесты и печатает итог.
//   argc, argv - параметры командной строки (--exe <путь к rus1scan.exe>).
int wmain(int argc, wchar_t* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    if (!AfxWinInit(GetModuleHandleW(NULL), NULL, GetCommandLineW(), 0))
    {
        std::printf("AfxWinInit failed\n");
        return 1;
    }
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    std::wstring exePath;
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (_wcsicmp(argv[i], L"--exe") == 0)
        {
            exePath = argv[i + 1];
        }
    }

    struct TestCase
    {
        const char* name;
        std::function<void()> run;
    };
    std::vector<TestCase> tests =
    {
        { "TomlParse", TestTomlParse },
        { "TomlErrors", TestTomlErrors },
        { "TomlQuote", TestTomlQuote },
        { "Strutil", TestStrutil },
        { "WiaNames", TestWiaNames },
        { "ConfigRoundTrip", TestConfigRoundTrip },
        { "ConfigErrors", TestConfigErrors },
        { "FindBinding", TestFindBinding },
        { "SaveFormats", TestSaveFormats },
        { "RunActionSave", TestRunActionSave },
    };
    if (!exePath.empty())
    {
        tests.push_back({ "ExeProcess", [&exePath]() { TestExeProcess(exePath); } });
    }

    for (const auto& test : tests)
    {
        const int failedBefore = g_failed;
        std::printf("[ RUN  ] %s\n", test.name);
        test.run();
        std::printf("[ %s ] %s\n", g_failed == failedBefore ? " OK " : "FAIL", test.name);
    }

    std::printf("\n%d checks passed, %d failed\n", g_passed, g_failed);
    CoUninitialize();
    return g_failed == 0 ? 0 : 1;
}
