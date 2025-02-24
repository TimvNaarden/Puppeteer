#pragma once

#include "Event/Event.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>
#include <string>

namespace Puppeteer 
{
    struct WindowProps
    {
        std::string Title = "Puppeteer";
        uint32_t Width = 1280;
        uint32_t Height = 720;
    };
    static void EventCallbackFn1(Event& e) {
        std::cout << e.ToString() << std::endl;
    }

    class Window
    {
    public:

        
        using EventCallbackFn = std::function<void(Event&)>;
        EventCallbackFn m_EventCallback = EventCallbackFn1;


        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        virtual std::string GetTitle() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void* GetNativeWindow() const = 0;

        static Scope<Window> Create(const WindowProps& props = WindowProps());
    };
}
