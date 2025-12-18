#include "pewpch.h"
#include "Application.h"
#include "Events/ApplicationEvent.h" 
#include "PewPew/Log.h"
namespace PewPew
{
	Application::Application()
	{
	}
	Application::~Application()
	{
	}
	void Application::Run()
	{
		WindowCloseEvent e;
		if (e.IsInCategory(EventCategoryApplication))
		{
			PEW_TRACE(e.ToString());
		}
		if (e.IsInCategory(EventCategoryInput))
		{
			PEW_TRACE(e.ToString());
		}

		while (true);
	}
}