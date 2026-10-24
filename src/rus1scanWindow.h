// rus1scanWindow.h: окно настроек rus1scan (открывается двойным кликом по rus1scan.exe).
//
// Пользователь выбирает устройство и событие (кнопку), затем действие: печать,
// запуск программы, ничего или сохранение в папку (BMP или PDF). Кнопки OK/Apply
// сохраняют настройки в rus1scan.toml; работающая служба перечитывает файл сама.

#pragma once

#include <string>
#include <vector>

#include "resource.h"
#include "config.h"
#include "rus1scan.h"

class CRus1scanWindow : public CDialog
{
public:
    // Создаёт окно настроек.
    explicit CRus1scanWindow(CWnd* pParent = NULL);

    enum { IDD = IDD_RUS1SCAN_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnOK();

    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnDeviceChanged();
    afx_msg void OnEventChanged();
    afx_msg void OnShowDisconnectedChanged();
    afx_msg void OnActionChanged();
    afx_msg void OnBnClickedButtonBrowse();
    afx_msg void OnBnClickedButtonApply();

    DECLARE_MESSAGE_MAP()

private:
    void BuildDeviceList();
    void FillDeviceCombo(const std::wstring& preferredId);
    void FillEventCombo();
    void FillProgramCombo();
    const WiaDeviceInfo* CurrentDevice() const;
    const WiaEventInfo* CurrentEvent() const;
    void LoadBindingToControls();
    void StoreControlsToBinding();
    Rus1Action GetSelectedAction() const;
    void SetSelectedAction(Rus1Action action);
    void UpdateControlStates();
    bool SaveSettings();
    void SetStatus(const std::wstring& text);
    std::wstring GetServiceStateText() const;

    CComboBox     m_ComboBoxDevices;
    CComboBox     m_ComboBoxEvents;
    CComboBox     m_ComboBoxPrinters;
    CComboBox     m_ComboBoxStart;
    CEdit         m_editSaveDirectory;
    CButton       m_checkPdf;
    CButton       m_checkShowDisconnected;
    CStatic       m_DescrText;
    CStatic       m_StatusText;
    CToolTipCtrl  m_ToolTip;

    Rus1Config                 m_config;        // Редактируемые настройки
    std::wstring               m_configPath;
    std::vector<WiaDeviceInfo> m_devices;       // Устройства в списке (подключённые, из конфига, "любое")
    int                        m_currentDevice = -1;
    int                        m_currentEvent = -1;
};
