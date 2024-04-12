#pragma once

#include "Core/Layer.h"
#include "Renderer/Framebuffer.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <mutex>
#include <TCP/TCPClient.h>

namespace Puppeteer {
	typedef struct GridImage {
		ImageData data;
		char* name;
		char* ip;
	};

	class GridLayer : public Layer {
	public:
		GridLayer();
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float dt) override;
		virtual void OnImGuiRender() override;

	private:
		Ref<Framebuffer> m_Framebuffer;
		uint32_t m_TextureID;
		
		std::mutex m_Mutex;

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportOffset = { 0.0f, 0.0f };
		bool m_First = true;

		int m_LayerNumber;

		std::time_t m_Lastrun;
		void GetImages();

		float m_Fps;
		
		std::map<std::string, Networking::TCPClient> m_Clients;

		std::queue<std::vector<GridImage>> m_Images;
		std::vector<GridImage> m_CurrentImages;

		ImTextureID blackImage;
	};
}