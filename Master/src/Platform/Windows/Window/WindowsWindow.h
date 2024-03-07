#pragma once

#include "Core/Window.h"

namespace Puppeteer
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		virtual void OnUpdate() override;

		virtual inline std::string GetTitle() const override	{ return m_Data.Title; }
		virtual inline uint32_t GetWidth() const override		{ return m_Data.Width; }
		virtual inline uint32_t GetHeight() const override		{ return m_Data.Height; }

		virtual inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled) override;
		virtual bool IsVSync() const override { return m_Data.VSync; }

		virtual inline void* GetNativeWindow() const override { return m_Window; }
	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			unsigned int Width = 0, Height = 0;
			bool VSync = false;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}
