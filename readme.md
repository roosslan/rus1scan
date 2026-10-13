# rus1scan

Служба Windows для обработки кнопок сканера (WIA). По нажатию кнопки сканер
сканирует, а результат сохраняется в директорию (GIF, BMP или PDF), печатается или
передаётся указанной программе. Работает без входа пользователя в Windows.

## Режимы запуска

| Команда | Что происходит |
|---|---|
| `rus1scan.exe` (двойной клик) | Окно настроек; OK / Apply сохраняют `rus1scan.toml` |
| `rus1scan.exe` (запуск службой) | Служба `rus1scan`, без окон |
| `rus1scan.exe /install` | Установка и запуск службы (нужны права администратора) |
| `rus1scan.exe /uninstall` | Остановка и удаление службы |
| `rus1scan.exe /console` | То же, что служба, но в консоли; выход — Enter или Ctrl+C |
| `rus1scan.exe /process файл.bmp {GUID события} [ID устройства]` | Выполнить действие привязки над готовым BMP (проверка без сканера) |
| `rus1scan.exe /StiDevice:… /StiEvent:…` | Запуск самой WIA по событию-«действию» |

## Настройки: rus1scan.toml

Файл лежит рядом с `rus1scan.exe`, кодировка UTF-8. Служба отслеживает его и
перечитывает после каждого сохранения, перезапуск не нужен.

```toml
[common]
filePrefix = 'Сканирование_'      # префикс имён файлов
filetype = 'gif'                  # gif (до 256 цветов, малый размер) | bmp (без потерь)

[gui]
showDisconnectedDevices = false   # показывать в окне отключённые устройства

[wia]                             # WIA-свойства перед каждым сканированием
WIA_IPS_XRES = 300
WIA_IPS_YRES = 300
WIA_IPA_DATATYPE = 'WIA_DATA_GRAYSCALE'
WIA_DPS_DOCUMENT_HANDLING_SELECT = 'FEEDER|DUPLEX'

[[binding]]                       # кнопка устройства -> действие
deviceId = '{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}\0001'
eventGuid = '{C4E6B54E-0AA0-4490-9F6C-C93F68347A13}'
action = 'save'                   # nothing | save | print | start
folder = 'C:\Scans\%d.%m.%Y\'     # %d %m %Y %y %H %M %S - дата и время скана
pdf = true                        # все страницы в один PDF; false - файл filetype на каждую страницу
printer = ''                      # для print; '' - принтер по умолчанию
program = ''                      # для start; %1 - путь к файлу
```

В секции `[wia]` ключом может быть любое свойство из `WIA_CONSTANTS.TXT.H`
(имя, отображаемое имя или номер). Значение — число, `true`/`false`, константа
WIA (несколько — через `|`), дробное число или текст; оно приводится к типу
свойства конкретного сканера. Если задано разрешение без области сканирования,
сканируется всё стекло.

Таблица имён WIA формируется из `WIA_CONSTANTS.TXT.H` и `WiaDef.h` (Windows SDK):

```
python tools\gen_wia_names.py
```

## Сборка

Visual Studio 2022 (v143), MFC, конфигурации Debug/Release для Win32
(`litePDF.dll` — 32-битная). После сборки рядом с exe копируются `litePDF.dll`
и, если его там нет, шаблон `rus1scan.toml`.

## Журнал

`logs\rus1scan_ГГГГ-ММ-ДД.log` рядом с exe (при отсутствии прав на запись —
`%TEMP%\rus1scan\logs`). В каждой строке указаны режим работы, PID и место в коде.
