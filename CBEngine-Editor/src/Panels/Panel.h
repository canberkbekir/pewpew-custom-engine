#pragma once

#include "CBEngine/Events/Event.h"

#include <string>

namespace CB
{
	class Panel
	{
	public:
		Panel(const std::string& name,bool visible = true)
			: m_Name(name), m_Visible(visible)
		{
		}
		virtual ~Panel() = default;

		virtual void OnImGuiRender() = 0;
		virtual void OnEvent(Event& e)
		{
		}

		void SetVisible(bool visible) { m_Visible = visible; }
		bool IsVisible() const { return m_Visible; }
		void ToggleVisibility() { m_Visible = !m_Visible; }
		bool* GetVisiblePtr() { return &m_Visible; }

		const std::string& GetName() const { return m_Name; }
	protected:
		std::string m_Name;
		bool m_Visible = true;
	};
}