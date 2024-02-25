#pragma once

#include "Event/ApplicationEvent.h"
#include "Event/MouseEvent.h"
#include "Event/KeyEvent.h"

#include "Core/Layer.h"

namespace Puppeteer
{
	class ImGuiLayer : public Layer
	{
	public:
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		ImGuiID dockspace_id;
		ImGuiID dock_main_id;
		ImGuiID dock_left_id;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }

	private:
		void SetThemeColors();
	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;
		int m_Initialized = 0;
		
	};
}