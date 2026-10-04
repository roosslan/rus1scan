// actions.h: действия над отсканированными изображениями - сохранение (BMP/PDF),
// печать и запуск программы, а также выполнение привязки "событие -> действие".

#pragma once

#include <string>
#include <vector>

#include "config.h"
#include "rus1scan.h"

namespace actions
{
    // Список принтеров системы (локальные и подключённые).
    std::vector<std::wstring> EnumeratePrinters();

    // Имя принтера по умолчанию (пусто, если не задан).
    std::wstring GetDefaultPrinterName();

    // Загружает .bmp-файл с диска (для режима /process).
    bool LoadBmpFile(const std::wstring& path, ScannedImage& image, std::wstring& error);

    // Сохраняет страницы как отдельные .bmp-файлы.
    bool SaveImagesAsBmp(const std::vector<ScannedImage>& images, const std::wstring& folder,
                         const std::wstring& prefix, std::vector<std::wstring>& files, std::wstring& error);

    // Сохраняет все страницы в один PDF-файл.
    bool SaveImagesAsPdf(const std::vector<ScannedImage>& images, const std::wstring& folder,
                         const std::wstring& prefix, std::wstring& file, std::wstring& error);

    // Печатает страницы на принтере.
    bool PrintImages(const std::vector<ScannedImage>& images, const std::wstring& printer, std::wstring& error);

    // Запускает программу для каждого файла.
    bool StartProgram(const std::wstring& commandTemplate, const std::vector<std::wstring>& files, std::wstring& error);

    // Выполняет действие привязки над уже полученными страницами.
    bool RunActionOnImages(const EventBinding& binding, const Rus1Config& config,
                           const std::vector<ScannedImage>& images);

    // Полный цикл обработки события: сканирование + действие.
    bool ExecuteBinding(const EventBinding& binding, const Rus1Config& config,
                        const std::wstring& deviceId, const std::wstring& itemName);
}
