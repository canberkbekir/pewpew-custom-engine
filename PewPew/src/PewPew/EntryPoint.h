#pragma once

#ifdef PEW_PLATFORM_WINDOWS

extern PewPew::Application* PewPew::CreateApplication();

int main(int argc, char** argv)
{
    PewPew::Log::Init();
    PEW_ERROR("Initialized Log!");
    auto app = PewPew::CreateApplication();
    app->Run();
    delete app;
}
#endif
