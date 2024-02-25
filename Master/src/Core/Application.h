#pragma once

#include "Core/LayerStack.h"
#include "Core/Window.h"
#include "Event/ApplicationEvent.h"
#include "Event/Event.h"
#include "ImGui/ImGuiLayer.h"
#include <queue>

#include <GLFW/glfw3.h>

namespace Puppeteer
{
	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const { return Args[index]; }
	};

	class Application
	{
	public:
		Application(const WindowProps& props = WindowProps(), ApplicationCommandLineArgs args = ApplicationCommandLineArgs());
		virtual ~Application() = default;

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		void RemoveLayer(Layer* layer);

		void Run();
		void Exit() { m_Running = false; }

		inline Window& GetWindow() { return *m_Window; }
		inline ImGuiLayer* GetImGuiLayer() const { return m_ImGuiLayer; }

		inline static Application& Get() { return *s_Instance; }	
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		Scope<Window> m_Window;
		LayerStack m_LayerStack;
		std::queue<Layer*> m_LayersToRemove;
		std::queue<Layer*> m_LayersToPush;
		ImGuiLayer* m_ImGuiLayer;

		bool m_Running = false;
		bool m_Minimized = false;
	private:
		static Application* s_Instance;
	};
}