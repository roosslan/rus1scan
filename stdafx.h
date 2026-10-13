// stdafx.h: общий заголовок проекта rus1scan.
// Подключает MFC, WIA, Win32 API и STL, которые используются почти во всех модулях.

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN                // Исключаем редко используемые части заголовков Windows
#endif

#include <afxwin.h>                 // Ядро MFC
#include <afxext.h>                 // Расширения MFC
#include <afxdlgs.h>                // CFolderPickerDialog
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                 // Common Controls (CToolTipCtrl)
#endif

#include <atlbase.h>
#include <atlcomcli.h>              // CComPtr, CComQIPtr, CComBSTR
#include <winsvc.h>
#include <winspool.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <sddl.h>
#include <wia.h>
#include <sti.h>

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <filesystem>
#include <sstream>
#include <type_traits>

#pragma comment(lib, "wiaguid.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "advapi32.lib")

#include "logger.h"

#define COUNTOF(x) (sizeof(x) / sizeof(*(x)))
