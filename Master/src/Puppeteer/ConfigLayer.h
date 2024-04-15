#pragma once

namespace Puppeteer {
	class ConfigLayer : public Layer {
		public:
			GLuint m_Texture;
			PCInfo pc;

			ConfigLayer();
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
			bool m_First = true;

			int m_LayerNumber;

			float m_Fps;
	};
}