#include "pch.h"
#include "LayerStack.h"

namespace Puppeteer
{
	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
		{
			layer->OnDetach();
			delete layer;
		}
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;
	}


	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
		{
			if (*it == layer)
			{
				layer->OnDetach();
				it = decltype(it)(m_Layers.erase(std::next(it).base())); // erase returns iterator to the next valid element
				m_LayerInsertIndex = std::distance(m_Layers.begin(), it.base()); // update m_LayerInsertIndex
				break; // break the loop after erasing the element
			}
		}
		delete layer;

	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			overlay->OnDetach();
			m_Layers.erase(it);
		}
	}
}