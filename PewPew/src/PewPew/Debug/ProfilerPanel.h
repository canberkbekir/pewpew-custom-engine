#pragma once

#include "PewPew/Events/Event.h"
#include "PewPew/Events/KeyEvent.h"

namespace PewPew
{
	class ProfilerPanel
	{
	public:
		ProfilerPanel() = default;
		~ProfilerPanel() = default;

		void OnImGuiRender();
		void OnEvent(Event& e);

		void SetVisible(bool visible) { m_Visible = visible; }
		bool IsVisible() const { return m_Visible; }
		void ToggleVisibility() { m_Visible = !m_Visible; }

	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		void RenderScopeTree();

	private:
		bool m_Visible = false;
	};
}
