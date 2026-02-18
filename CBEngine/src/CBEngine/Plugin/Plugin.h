#pragma once

#include "CBEngine/Core/Core.h"

namespace CB
{
	class Application;

	class CB_API IPlugin
	{
	public:
		virtual ~IPlugin() = default;
		virtual const char* GetName() const = 0;
		virtual void OnLoad(Application& app) = 0;
		virtual void OnUnload() = 0;
	};

	// Plugins export this function:
	// extern "C" CB_API IPlugin* CreatePlugin();
}
