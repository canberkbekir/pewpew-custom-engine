#pragma once
#include "Core.h"
#include "Events/Event.h"
namespace PewPew
{
	class PEW_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
	};

	//To be defined in CLIENT
	Application* CreateApplication();
}