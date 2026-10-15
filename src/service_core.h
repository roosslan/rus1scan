// service_core.h: рабочая часть службы rus1scan (без окон). Используется и настоящей
// службой Windows, и отладочным режимом "rus1scan.exe /console".
//
// Что делает:
//   - читает rus1scan.toml и регистрируется в WIA на события из привязок [[binding]];
//   - по событию сканирует и выполняет действие (сохранить / напечатать / запустить);
//   - для событий, которые устройство отдаёт только как "действие" (WIA_ACTION_EVENT),
//     регистрирует постоянный обработчик: WIA сама запустит rus1scan.exe /StiDevice:... /StiEvent:...;
//   - следит за файлом конфига и перечитывает его после сохранения из окна настроек.

#pragma once

#include <deque>
#include <map>
#include <memory>
#include <mutex>

#include "config.h"
#include "EventCallback.h"

// Потокобезопасная очередь событий WIA: события приходят в потоках RPC,
// а обрабатываются в рабочем потоке службы.
class CEventQueue
{
public:
    CEventQueue();
    ~CEventQueue();

    CEventQueue(const CEventQueue&) = delete;
    CEventQueue& operator=(const CEventQueue&) = delete;

    // Добавляет событие в очередь и будит рабочий поток.
    void Push(const WiaEventNotification& event);

    // Забирает событие из очереди; false - очередь пуста.
    bool Pop(WiaEventNotification& event);

    // Событие Win32, которое взводится при добавлении в очередь.
    HANDLE SignalHandle() const { return m_signal; }

private:
    std::mutex                       m_mutex;
    std::deque<WiaEventNotification> m_items;
    HANDLE                           m_signal;
};

class CServiceCore
{
public:
    // Создаёт ядро службы (см. service_core.cpp).
    explicit CServiceCore(HANDLE stopEvent);
    ~CServiceCore();

    CServiceCore(const CServiceCore&) = delete;
    CServiceCore& operator=(const CServiceCore&) = delete;

    // Запускает рабочий поток; false - поток не создан.
    bool Start();

    // Ждёт завершения рабочего потока.
    void Wait();

    // Код завершения рабочего потока (0 - нормальное завершение).
    DWORD ExitCode() const { return m_exitCode; }

private:
    static DWORD WINAPI ThreadProc(LPVOID param);
    DWORD Run();

    bool ReloadConfig(bool initial);
    bool GetConfigWriteTime(FILETIME& time) const;
    void RegisterEvents();
    void UnregisterEvents();
    void RegisterPersistentHandlers();
    void UnregisterPersistentHandlers();
    void ProcessQueue();
    void HandleEvent(const WiaEventNotification& event);

    // Одна постоянная регистрация обработчика (RegisterEventCallbackProgram).
    struct PersistentRegistration
    {
        std::wstring deviceId;
        GUID         eventGuid;
    };

    HANDLE                              m_stopEvent;    // Не принадлежит объекту
    HANDLE                              m_thread = NULL;
    DWORD                               m_exitCode = 0;
    std::wstring                        m_configPath;
    FILETIME                            m_configTime = {};
    Rus1Config                          m_config;
    std::shared_ptr<CEventQueue>        m_queue;
    CComPtr<IWiaDevMgr>                 m_manager;
    CComPtr<WiaWrap::CEventCallback>    m_callback;
    std::vector<PersistentRegistration> m_persistent;
    std::map<std::wstring, ULONGLONG>   m_lastEventTick;   // Подавление повторов одного события
};

// Настраивает безопасность COM процесса так, чтобы служба WIA (LocalService) могла
// вызывать наш приёмник событий. Вызывается один раз, до первого обращения к WIA.
HRESULT InitializeComSecurityForService();
