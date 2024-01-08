#include "pch.h"
#include "Window.h"

#ifdef PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif
#ifdef PLATFORM_LINUX
#include "Platform/Linux/LinuxWindow.h"
#endif

namespace Puppeteer 
{
    Scope<Window> Window::Create(const WindowProps& props)
    {
        #ifdef PLATFORM_WINDOWS
            return CreateScope<WindowsWindow>(props);
        #else
        #ifdef PLATFORM_LINUX
            return CreateScope<LinuxWindow>(props);
        #else
            return nullptr;
        #endif
        #endif
    }
}