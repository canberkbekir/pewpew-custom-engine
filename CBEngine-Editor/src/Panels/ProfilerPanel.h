#pragma once

#include "Panel.h"
#include "CBEngine/Events/KeyEvent.h"
#include "CBEngine/Input/KeyCodes.h"

#include <string>

namespace CB
{
	class ProfilerPanel : public Panel
	{
	public:
		ProfilerPanel()
			: Panel("Profiler", false)
		{
		}

		void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		void RenderRecordingControls();
		void RenderScopeTree();

		// Export feedback
		std::string m_ExportMessage;
		float m_ExportMessageTimer = 0.0f;
	};
}
