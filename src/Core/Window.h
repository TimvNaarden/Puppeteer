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

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

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

    //void streamScreen(GLFWwindow* window);
    //GLFWwindow* createGLFWWindow(int width, int height, const char* title);

    //void destroyGLFWWindow(GLFWwindow* window);

    //void setupImGui(GLFWwindow* window);

    //void renderImGui(GLFWwindow* window);

    //void destroyImGui();

    //vec2 getScreenSize();

    //void displayPicture(ImTextureID textureID);

    //vec2 getWindowSize();

    //void startRender();
}
