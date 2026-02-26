#pragma once
#include "CBEngine/Core/Layer.h"
#include "CBEngine/Events/ApplicationEvent.h"
#include "CBEngine/Events/KeyEvent.h"
#include "CBEngine/Events/MouseEvent.h"

namespace CB
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() override;

		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(Event& e) override;
		void OnImGuiRender() override;

		void Begin();
		void End();
	private:
		float m_Time = 0.0f;
	};
}