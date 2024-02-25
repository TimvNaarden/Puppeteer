#include "pch.h"
#include "InfoLayer.h"
#include "imgui.h"
#include <imgui_internal.h>

namespace Puppeteer {

	InfoLayer::InfoLayer() : m_ActiveIndex(0) {}

	void InfoLayer::OnAttach()
	{
		FramebufferSpecification spec;
		spec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::Depth };
		spec.Width = 1280;
		spec.Height = 720;
		m_Framebuffer = CreateRef<Framebuffer>(spec);

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	}


	void InfoLayer::OnDetach()
	{
		glDeleteTextures(1, &m_TextureID);
	}

	void InfoLayer::OnUpdate(float dt)
	{
		if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		m_Framebuffer->Bind();

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		m_Framebuffer->Unbind();
	}
	void InfoLayer::OnImGuiRender()
	{
		if (PcInfos.size() == 0) {
			return;
		}
		ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
		ImGui::Begin("Info");

		std::stringstream ss;
		ss << PcInfos[m_ActiveIndex];
		ImGui::Text(ss.str().c_str());
		int focused = ImGui::IsWindowFocused();
		ImGui::End();

		if ((!focused && !m_ViewportFocused) && !m_ViewportHovered) {
			return; 
		}

		ImGui::SetNextWindowDockID(1, ImGuiCond_FirstUseEver);
		ImGui::Begin("Select PC:");

		int index = 0;
		for (PCInfo info : PcInfos) {
			if (ImGui::Button(info.m_Systemname.c_str())) {
				m_ActiveIndex = index;
			}
			index++;
		}
		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		ImGui::End();
	}
	void InfoLayer::OnEvent(Event& event)
	{

	}
}