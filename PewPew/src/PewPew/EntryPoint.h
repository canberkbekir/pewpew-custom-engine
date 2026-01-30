#pragma once

#include "Debug/Instrumentor.h"

#ifdef PEW_PLATFORM_WINDOWS

extern PewPew::Application* PewPew::CreateApplication();

int main(int argc, char** argv)
{
    PewPew::Log::Init();
    PEW_CORE_INFO("Initialized Log!");

    PEW_PROFILE_BEGIN_SESSION("Startup", "PewPewProfile-Startup.json");
    auto app = PewPew::CreateApplication();
    PEW_PROFILE_END_SESSION();

    PEW_PROFILE_BEGIN_SESSION("Runtime", "PewPewProfile-Runtime.json");
    app->Run();
    PEW_PROFILE_END_SESSION();

    PEW_PROFILE_BEGIN_SESSION("Shutdown", "PewPewProfile-Shutdown.json");
    delete app;
    PEW_PROFILE_END_SESSION();
}
#endif
