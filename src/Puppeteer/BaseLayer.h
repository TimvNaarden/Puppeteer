#pragma once

#include "Core/Layer.h"
#include "Renderer/Framebuffer.h"

#include <glm/glm.hpp>

namespace Puppeteer
{
	class BaseLayer : public Layer
	{
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float dt) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;
	private:
		Ref<Framebuffer> m_Framebuffer;
		uint32_t m_TextureID;

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportOffset = { 0.0f, 0.0f };

		float m_Fps;
	};
}