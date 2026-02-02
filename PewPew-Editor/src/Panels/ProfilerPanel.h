#pragma once

#include "Panel.h"
#include "PewPew/Events/KeyEvent.h"
#include "PewPew/Input/KeyCodes.h"

namespace PewPew
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
		void RenderScopeTree();
	};
}