#pragma once

#include "Socket/Socket.h"
#include "Core/Layer.h"
#include "Renderer/Framebuffer.h"

#include "Platform/Windows/DirectX11/DirectX11.h"
#include "Platform/Windows/PCInfo/PCInfo.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <future>
#include <thread>
#include <queue>	
#include <mutex>

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
		Socket* m_Socket;
		std::string m_Credentials;

		char* m_Name;

		bool m_Initialized;

		PuppetLayer() = delete;
		PuppetLayer(char* Ip, std::string Credentials);
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


	};
}