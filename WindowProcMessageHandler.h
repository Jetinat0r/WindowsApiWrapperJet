#pragma once
#include "framework.h"
#include <shared_mutex>
#include <vector>
#include <map>

#ifdef WINDOWSAPIWRAPPERJET_EXPORTS
#define WINDOWSAPIWRAPPERJET_API __declspec(dllexport)
#else
#define WINDOWSAPIWRAPPERJET_API __declspec(dllimport)
#endif

//Matches WNDPROC (minus an __stdcall), but I own it :)
typedef LRESULT (*WNDPROC_MESSAGE_HANDLER)(HWND, UINT, WPARAM, LPARAM);

struct WindowProcMessageHandler
{
public:
    HWND windowHandle;
    bool assignedToWndProc;
    WNDPROC originalWndProc;
    std::vector<WNDPROC_MESSAGE_HANDLER> callbackMessageHandlers;
};

//Create & Destroy WindowProcMessageHandler, and add it to the registeredWindowProcMessageHandlers map
extern "C" WINDOWSAPIWRAPPERJET_API bool CreateWindowProcMessageHandler(HWND _windowHandle);
extern "C" WINDOWSAPIWRAPPERJET_API bool DestroyWindowProcMessageHandler(HWND _windowHandle);

//Adds/removes a callback message handler to an existing WindowProcMessageHandler
extern "C" WINDOWSAPIWRAPPERJET_API bool AddMessageHandler(HWND _windowHandle, WNDPROC_MESSAGE_HANDLER _messageHandler);
extern "C" WINDOWSAPIWRAPPERJET_API bool RemoveMessageHandler(HWND _windowHandle, WNDPROC_MESSAGE_HANDLER _messageHandler);

//Takes over / relinquish control over _windowHandle's WNDPROC. Requires a WindowProcMessageHandler to be registered
extern "C" WINDOWSAPIWRAPPERJET_API bool AssignWindowProc(HWND _windowHandle);
extern "C" WINDOWSAPIWRAPPERJET_API bool RemoveWindowProc(HWND _windowHandle);


LRESULT static HandleMessage(HWND _windowHandle, UINT _message, WPARAM _wparam, LPARAM _lParam);