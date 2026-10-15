// rus1scanService.h: приложение rus1scan (CWinApp) - выбор режима работы.
//
// Режимы запуска rus1scan.exe:
//   без параметров, запущен SCM          - служба Windows "rus1scan" (без окон);
//   без параметров, двойной клик по exe  - окно настроек, сохраняет rus1scan.toml;
//   /install, /uninstall                 - установка / удаление службы (нужны права администратора);
//   /console                             - то же, что служба, но в консоли (для отладки);
//   /StiDevice:<id> /StiEvent:<guid>     - запуск самой WIA по событию-"действию" (без окон);
//   /process <файл.bmp> <GUID события> [ID устройства] - выполнить действие привязки
//                                          над готовым BMP (проверка без сканера);
//   /quiet                               - не показывать окна с сообщениями (для /install, /uninstall).

#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file
#endif

#include <string>
#include <vector>

class CRus1scanService : public CWinApp
{
public:
    CRus1scanService();

    virtual BOOL InitInstance();
    virtual int ExitInstance();

private:
    int RunServiceOrGui();
    int RunGui();
    int RunConsole();
    int RunInstall();
    int RunUninstall();
    int RunStiLaunch(const std::wstring& deviceId, const std::wstring& eventGuid);
    int RunProcessFile(const std::vector<std::wstring>& args);
    void ShowMessage(const std::wstring& text, bool isError);

    int  m_exitCode = 0;
    bool m_quiet = false;

    DECLARE_MESSAGE_MAP()
};
