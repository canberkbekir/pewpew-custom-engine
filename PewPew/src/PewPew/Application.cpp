#include "pewpch.h"
#include "Application.h"
#include "Events/ApplicationEvent.h"  
namespace PewPew
{
#define BIND_EVENT_FN(fn) std::bind(&Application::fn, this, std::placeholders::_1)
	
	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window-> SetEventCallback(BIND_EVENT_FN(OnEvent));
	}
	
	Application::~Application()
	{
	}
	
	void Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();
		}
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		PEW_CORE_TRACE(e.ToString());
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
 
}
