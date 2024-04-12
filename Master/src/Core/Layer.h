#pragma once

#include "Event/Event.h"

#include <string>

namespace Puppeteer
{
	class Layer
	{
	public:
		virtual ~Layer() = default;
		char* m_Name;
		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnUpdate(float dt) {};
		virtual void OnImGuiRender() {};
		virtual void OnEvent(Event& event) {};
	};
}
