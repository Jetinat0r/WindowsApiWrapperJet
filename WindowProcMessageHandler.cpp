#include "pch.h"
#include "WindowProcMessageHandler.h"

//Map of all windows we should be looking for messages from and what handlers each window desires
std::map<HWND, WindowProcMessageHandler> registeredWindowProcMessageHandlers;
//Mutex ensuring thread safety of operations to the window handler map
std::shared_mutex msgHandlerLock;

WINDOWSAPIWRAPPERJET_API bool CreateWindowProcMessageHandler(HWND _windowHandle)
{
    std::unique_lock<std::shared_mutex> _ul(msgHandlerLock);

    if (registeredWindowProcMessageHandlers.contains(_windowHandle))
    {
        return false;
    }

    WindowProcMessageHandler _newWindowProcMessageHandler = { 0 };
    _newWindowProcMessageHandler.windowHandle = _windowHandle;
    _newWindowProcMessageHandler.assignedToWndProc = false;
    _newWindowProcMessageHandler.originalWndProc = nullptr;
    _newWindowProcMessageHandler.callbackMessageHandlers.clear();
    registeredWindowProcMessageHandlers.emplace(_windowHandle, _newWindowProcMessageHandler);

    return true;
}

WINDOWSAPIWRAPPERJET_API bool DestroyWindowProcMessageHandler(HWND _windowHandle)
{
    bool _result = false;

    std::unique_lock<std::shared_mutex> _ul(msgHandlerLock);

    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator != registeredWindowProcMessageHandlers.end())
    {
        WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;
        //If we're assigned to a WNDPROC, we should remove ourselves first
        //No need to clean our callbackMessageHandlers as they'll be destroyed anyways, and as callbacks there's no hard link from the other side
        if (!_windowProcMessageHandler->assignedToWndProc)
        {
            SetWindowLongPtr(_windowHandle, GWLP_WNDPROC, (LONG_PTR)_windowProcMessageHandler->originalWndProc);
            _windowProcMessageHandler->originalWndProc = nullptr;
            _windowProcMessageHandler->assignedToWndProc = false;
        }

        registeredWindowProcMessageHandlers.erase(_windowProcMessageHandlerIterator);
        _result = true;
    }

    return _result;
}

WINDOWSAPIWRAPPERJET_API bool AddMessageHandler(HWND _windowHandle, WNDPROC_MESSAGE_HANDLER _messageHandler)
{
    std::unique_lock<std::shared_mutex> _ul(msgHandlerLock);

    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        msgHandlerLock.unlock();
        return false;
    }
    WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

    _windowProcMessageHandler->callbackMessageHandlers.push_back(_messageHandler);

    return true;
}

WINDOWSAPIWRAPPERJET_API bool RemoveMessageHandler(HWND _windowHandle, WNDPROC_MESSAGE_HANDLER _messageHandler)
{
    bool _result = false;

    std::unique_lock<std::shared_mutex> _ul(msgHandlerLock);

    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        return false;
    }
    WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

    for (std::vector<WNDPROC_MESSAGE_HANDLER>::iterator it = _windowProcMessageHandler->callbackMessageHandlers.begin(); it != _windowProcMessageHandler->callbackMessageHandlers.end(); it++)
    {
        if (*it == _messageHandler)
        {
            _windowProcMessageHandler->callbackMessageHandlers.erase(it);
            _result = true;
            break;
        }
    }

    return _result;
}

WINDOWSAPIWRAPPERJET_API bool AssignWindowProc(HWND _windowHandle)
{
    if (_windowHandle == nullptr)
    {
        return false;
    }

    std::unique_lock<std::shared_mutex> _ul(msgHandlerLock);

    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        return false;
    }
    WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

    //Don't re-assign ourselves to the WNDPROC
    if (_windowProcMessageHandler->assignedToWndProc)
    {
        return false;
    }

    _windowProcMessageHandler->originalWndProc = (WNDPROC)SetWindowLongPtr(_windowHandle, GWLP_WNDPROC, (LONG_PTR)HandleMessage);
    _windowProcMessageHandler->assignedToWndProc = true;

    return true;
}

WINDOWSAPIWRAPPERJET_API bool RemoveWindowProc(HWND _windowHandle)
{
    std::unique_lock<std::shared_mutex> _ul(msgHandlerLock);

    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        return false;
    }
    WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

    //We can't unassign ourselves if we never connected in the first place
    if (!_windowProcMessageHandler->assignedToWndProc)
    {
        return false;
    }

    SetWindowLongPtr(_windowHandle, GWLP_WNDPROC, (LONG_PTR)_windowProcMessageHandler->originalWndProc);
    _windowProcMessageHandler->originalWndProc = nullptr;
    _windowProcMessageHandler->assignedToWndProc = false;

    return true;
}

LRESULT static HandleMessage(HWND _windowHandle, UINT _message, WPARAM _wparam, LPARAM _lParam)
{
    std::shared_lock<std::shared_mutex> _sl(msgHandlerLock);

    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator != registeredWindowProcMessageHandlers.end())
    {
        WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

        //Call all function pointers
        for (std::vector<WNDPROC_MESSAGE_HANDLER>::iterator it = _windowProcMessageHandler->callbackMessageHandlers.begin(); it != _windowProcMessageHandler->callbackMessageHandlers.end(); it++)
        {
            //Dereference iterator and Call function pointer
            (*it)(_windowHandle, _message, _wparam, _lParam);
        }

        WNDPROC _wndProc = _windowProcMessageHandler->originalWndProc;
        
        //Call Original WndProc function if available, or call the default and use its return value
        if (_wndProc != nullptr)
        {
            return CallWindowProc(_wndProc, _windowHandle, _message, _wparam, _lParam);
        }
        else
        {
            return DefWindowProc(_windowHandle, _message, _wparam, _lParam);
        }
    }
    else
    {
        return DefWindowProc(_windowHandle, _message, _wparam, _lParam);
    }
}
