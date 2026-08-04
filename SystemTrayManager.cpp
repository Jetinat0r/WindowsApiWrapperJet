#include "pch.h"
#include "SystemTrayManager.h"

WINDOWSAPIWRAPPERJET_API bool AddTrayIcon(HWND _windowHandle, GUID _iconGuid, TCHAR* _iconHoverText, HICON _iconImageHandle)
{
    NOTIFYICONDATA _iconData = {};
    _iconData.cbSize = sizeof(_iconData);
    _iconData.hWnd = _windowHandle;
    _iconData.uFlags = NIF_GUID | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    _iconData.uVersion = NOTIFYICON_VERSION_4;
    _iconData.uCallbackMessage = WMAPP_NOTIFYCALLBACK;

    _iconData.guidItem = _iconGuid;
    
    //ARRAYSIZE handles size checking, so if someone passes a super long string we'll be OK
    if (StringCchCopy(_iconData.szTip, ARRAYSIZE(_iconData.szTip), _iconHoverText) != S_OK)
    {
        return false;
    }
    
    _iconData.hIcon = _iconImageHandle;

    if (!Shell_NotifyIcon(NIM_ADD, &_iconData))
    {
        return false;
    }

    if (!Shell_NotifyIcon(NIM_SETVERSION, &_iconData))
    {
        return false;
    }

    return true;
}

WINDOWSAPIWRAPPERJET_API bool ModifyTrayIconImage(GUID _iconGuid, HICON _iconImageHandle)
{
    NOTIFYICONDATA _iconData = {};
    _iconData.cbSize = sizeof(_iconData);
    _iconData.uFlags = NIF_GUID | NIF_ICON;

    _iconData.guidItem = _iconGuid;

    _iconData.hIcon = _iconImageHandle;

    if (!Shell_NotifyIcon(NIM_MODIFY, &_iconData))
    {
        return false;
    }

    return true;
}

WINDOWSAPIWRAPPERJET_API bool ModifyTrayIconHoverText(GUID _iconGuid, TCHAR* _iconHoverText)
{
    NOTIFYICONDATA _iconData = {};
    _iconData.cbSize = sizeof(_iconData);
    _iconData.uFlags = NIF_GUID | NIF_TIP;

    _iconData.guidItem = _iconGuid;

    //ARRAYSIZE handles size checking, so if someone passes a super long string we'll be OK
    if (StringCchCopy(_iconData.szTip, ARRAYSIZE(_iconData.szTip), _iconHoverText) != S_OK)
    {
        return false;
    }

    if (!Shell_NotifyIcon(NIM_MODIFY, &_iconData))
    {
        return false;
    }

    return true;
}

WINDOWSAPIWRAPPERJET_API bool RemoveTrayIcon(GUID _iconGuid)
{
    NOTIFYICONDATA _iconData = {};
    _iconData.cbSize = sizeof(_iconData);
    _iconData.uFlags = NIF_GUID;

    _iconData.guidItem = _iconGuid;

    if (!Shell_NotifyIcon(NIM_DELETE, &_iconData))
    {
        return false;
    }

    return true;
}