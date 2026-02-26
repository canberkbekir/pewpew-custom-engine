#pragma once

#include "CBEngine/Debug/Instrumentor.h"

#ifdef CB_PLATFORM_WINDOWS

extern CB::Application* CB::CreateApplication();

int main(int argc,char** argv)
{
	CB::Log::Init();
	CB_CORE_INFO("Initialized Log!");

	CB_PROFILE_BEGIN_SESSION("Startup", "CBEngineProfile-Startup.json");
	auto app = CB::CreateApplication();
	CB_PROFILE_END_SESSION();

	CB_PROFILE_BEGIN_SESSION("Runtime", "CBEngineProfile-Runtime.json");
	app->Run();
	CB_PROFILE_END_SESSION();

	CB_PROFILE_BEGIN_SESSION("Shutdown", "CBEngineProfile-Shutdown.json");
	delete app;
	CB_PROFILE_END_SESSION();
}
#endif