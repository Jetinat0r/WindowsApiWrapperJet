#include "pch.h"
#include "WindowProcMessageHandler.h"

//Map of all windows we should be looking for messages from and what handlers each window desires
std::recursive_mutex msgHandlerLock;
std::map<HWND, WindowProcMessageHandler> registeredWindowProcMessageHandlers;

WINDOWSAPIWRAPPERJET_API bool CreateWindowProcMessageHandler(HWND _windowHandle)
{
    msgHandlerLock.lock();
    if (registeredWindowProcMessageHandlers.contains(_windowHandle))
    {
        msgHandlerLock.unlock();
        return false;
    }

    WindowProcMessageHandler _newWindowProcMessageHandler = { 0 };
    _newWindowProcMessageHandler.windowHandle = _windowHandle;
    _newWindowProcMessageHandler.assignedToWndProc = false;
    _newWindowProcMessageHandler.originalWndProc = nullptr;
    _newWindowProcMessageHandler.callbackMessageHandlers.clear();
    registeredWindowProcMessageHandlers.emplace(_windowHandle, _newWindowProcMessageHandler);
    msgHandlerLock.unlock();
    return true;
}

WINDOWSAPIWRAPPERJET_API bool DestroyWindowProcMessageHandler(HWND _windowHandle)
{
    bool _result = false;

    msgHandlerLock.lock();

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

    msgHandlerLock.unlock();

    return _result;
}

WINDOWSAPIWRAPPERJET_API bool AddMessageHandler(HWND _windowHandle, WNDPROC_MESSAGE_HANDLER _messageHandler)
{
    msgHandlerLock.lock();
    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        msgHandlerLock.unlock();
        return false;
    }
    WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

    _windowProcMessageHandler->callbackMessageHandlers.push_back(_messageHandler);
    msgHandlerLock.unlock();

    return true;
}

WINDOWSAPIWRAPPERJET_API bool RemoveMessageHandler(HWND _windowHandle, WNDPROC_MESSAGE_HANDLER _messageHandler)
{
    bool _result = false;

    msgHandlerLock.lock();
    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        msgHandlerLock.unlock();
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

    msgHandlerLock.unlock();

    return _result;
}

WINDOWSAPIWRAPPERJET_API bool AssignWindowProc(HWND _windowHandle)
{
    if (_windowHandle == nullptr)
    {
        return false;
    }

    msgHandlerLock.lock();
    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        msgHandlerLock.unlock();
        return false;
    }
    WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

    //Don't re-assign ourselves to the WNDPROC
    if (_windowProcMessageHandler->assignedToWndProc)
    {
        msgHandlerLock.unlock();
        return false;
    }
    
    _windowProcMessageHandler->originalWndProc = (WNDPROC)SetWindowLongPtr(_windowHandle, GWLP_WNDPROC, (LONG_PTR)HandleMessage);
    _windowProcMessageHandler->assignedToWndProc = true;

    msgHandlerLock.unlock();
    return true;
}

WINDOWSAPIWRAPPERJET_API bool RemoveWindowProc(HWND _windowHandle)
{
    msgHandlerLock.lock();
    std::map<HWND, WindowProcMessageHandler>::iterator _windowProcMessageHandlerIterator = registeredWindowProcMessageHandlers.find(_windowHandle);
    if (_windowProcMessageHandlerIterator == registeredWindowProcMessageHandlers.end())
    {
        msgHandlerLock.unlock();
        return false;
    }
    WindowProcMessageHandler* _windowProcMessageHandler = &_windowProcMessageHandlerIterator->second;

    //We can't unassign ourselves if we never connected in the first place
    if (!_windowProcMessageHandler->assignedToWndProc)
    {
        msgHandlerLock.unlock();
        return false;
    }

    SetWindowLongPtr(_windowHandle, GWLP_WNDPROC, (LONG_PTR)_windowProcMessageHandler->originalWndProc);
    _windowProcMessageHandler->originalWndProc = nullptr;
    _windowProcMessageHandler->assignedToWndProc = false;

    msgHandlerLock.unlock();
    return true;
}

LRESULT static HandleMessage(HWND _windowHandle, UINT _message, WPARAM _wparam, LPARAM _lParam)
{
    LRESULT _result;

    msgHandlerLock.lock();
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
        //If we don't unlock before calling CallWindowProc, everything breaks. I have no idea why
        //msgHandlerLock.unlock();
        
        //Call Original WndProc function if available, or call the default and use its return value
        if (_wndProc != nullptr)
        {
            _result = CallWindowProc(_wndProc, _windowHandle, _message, _wparam, _lParam);
        }
        else
        {
            _result = DefWindowProc(_windowHandle, _message, _wparam, _lParam);
        }
    }
    else
    {
        //msgHandlerLock.unlock();
        _result = DefWindowProc(_windowHandle, _message, _wparam, _lParam);
    }

    msgHandlerLock.unlock();
    return _result;
}
