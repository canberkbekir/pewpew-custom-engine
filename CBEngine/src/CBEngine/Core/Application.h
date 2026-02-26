#pragma once
#include "Core.h"
#include "Layer.h"
#include "LayerStack.h"
#include "Window.h"
#include "CBEngine/Events/ApplicationEvent.h"
#include "CBEngine/Events/Event.h"
#ifdef CB_ENABLE_IMGUI
#include "CBEngine/ImGui/ImGuiLayer.h"
#endif

namespace CB
{
	class CB_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window& GetWindow() { return *m_Window; }

		void Close() { m_Running = false; }

		static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		Scope<Window> m_Window;
#ifdef CB_ENABLE_IMGUI
		ImGuiLayer* m_ImGuiLayer;
#endif
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
		static Application* s_Instance;
	};

	//To be defined in CLIENT
	Application* CreateApplication();
}