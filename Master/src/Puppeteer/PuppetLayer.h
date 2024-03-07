#pragma once

#include "TCP/TCPClient.h"
#include "Core/Layer.h"
#include "Core/Application.h"
#include "Renderer/Framebuffer.h"

#include "Platform/Windows/DirectX11/DirectX11.h"
#include "Platform/Windows/PCInfo/PCInfo.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "imgui_internal.h"
#include <imgui.h>

#include <future>
#include <thread>
#include <queue>	
#include <mutex>

#include <Windows.h>
#include <lz4.h>
#include "Store/StoreJson.h"

namespace Puppeteer
{
	typedef struct ImageData
	{
		int Width, Height;
		char* Texture;
	};

	class PuppetLayer : public Layer
	{
	public:
		PCInfo m_PCInfo;
	
		GLuint m_Texture;
		int lastWidth, lastHeight;
		std::queue<ImageData> m_Textures;

		std::mutex m_Mutex;
		
		char* m_Ip;
		Networking::TCPClient* m_Socket;
		Credentials_T m_Credentials;

		char* m_Name;

		bool m_Initialized;

		PuppetLayer() = delete;
		PuppetLayer(char* Ip, Credentials_T Creds);
		~PuppetLayer() {
			m_UpdatingTexture = false;
			if (m_Texture) glDeleteTextures(1, &m_Texture);
			if(m_TextureID) glDeleteTextures(1, &m_TextureID);
			while (m_Initialized) continue;
		}

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float dt) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;
	private:
		void UpdateTexture();
		bool m_UpdatingTexture;
		bool m_Input;
		bool m_UserInput;

		Ref<Framebuffer> m_Framebuffer;
		uint32_t m_TextureID;

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportOffset = { 0.0f, 0.0f };

		float m_Fps;

		bool KeyPressend(KeyPressedEvent& e);
		bool KeyReleased(KeyReleasedEvent& e);

		bool MouseMoves(MouseMovedEvent& e);
		bool MouseScrolled(MouseScrolledEvent& e);
		bool MouseButtonPressed(MouseButtonPressedEvent& e);
		bool MouseButtonReleased(MouseButtonReleasedEvent& e);
	};
}