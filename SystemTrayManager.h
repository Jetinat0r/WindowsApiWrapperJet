#pragma once
#include "framework.h"
#include <string>


#ifdef WINDOWSAPIWRAPPERJET_EXPORTS
#define WINDOWSAPIWRAPPERJET_API __declspec(dllexport)
#else
#define WINDOWSAPIWRAPPERJET_API __declspec(dllimport)
#endif

//Matches WNDPROC (minus an __stdcall), but I own it :)
typedef LRESULT(*WNDPROC_MESSAGE_HANDLER)(HWND, UINT, WPARAM, LPARAM);

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const WMAPP_HIDEFLYOUT = WM_APP + 2;

struct TrayIcon
{
public:
    PNOTIFYICONDATA trayIconData;
};

//C#:
//- GUID -> System.Runtime.NewGuid(); //NOTE: Define this once, then save & re-use it for future runs
//- HINSTANCE -> Marshal.GetHINSTANCE(); _executableHandle must be the handle of the executable OR dll within which the icon file is embedded
extern "C" WINDOWSAPIWRAPPERJET_API bool AddTrayIcon(HWND _windowHandle, GUID _iconGuid, TCHAR* _iconHoverText, HICON _iconImageHandle);
extern "C" WINDOWSAPIWRAPPERJET_API bool ModifyTrayIconImage(GUID _iconGuid, HICON _iconImageHandle);
extern "C" WINDOWSAPIWRAPPERJET_API bool ModifyTrayIconHoverText(GUID _iconGuid, TCHAR* _iconHoverText);
extern "C" WINDOWSAPIWRAPPERJET_API bool RemoveTrayIcon(GUID _iconGuid);
