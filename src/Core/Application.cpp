#include "pch.h"
#include "Application.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

namespace Puppeteer
{
    Application* Application::s_Instance = nullptr;

	Application::Application(const WindowProps& props, ApplicationCommandLineArgs args)
	{
        s_Instance = this;

        m_Window = Window::Create(props);
        m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
        m_Window->SetVSync(false);

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
	}

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.Handled)
                break;
            (*it)->OnEvent(e);
        }
    }
    
    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer* overlay)
    {
        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }

    void Application::Run()
	{
        m_Running = true;
        float lastFrameTime = 0.0f;
        while (m_Running)
        {
            float time = (float)glfwGetTime();
            float timestep = time - lastFrameTime;
            lastFrameTime = time;

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            for (Layer* layer : m_LayerStack)
                layer->OnUpdate(timestep);

            m_ImGuiLayer->Begin();
            for (Layer* layer : m_LayerStack)
                layer->OnImGuiRender();
            m_ImGuiLayer->End();

            m_Window->OnUpdate();
        }
	}

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;

        return false;
    }
}