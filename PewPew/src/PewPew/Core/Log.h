#pragma once

#include "spdlog/logger.h"

#include "Core.h"

namespace PewPew
{
    class Log
    {
    public:
        static void Init();

        static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

    private:
        static Ref<spdlog::logger> s_CoreLogger;
        static Ref<spdlog::logger> s_ClientLogger;
    };
}

#define PEW_CORE_ERROR(...) ::PewPew::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PEW_CORE_WARN(...)  ::PewPew::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PEW_CORE_INFO(...)  ::PewPew::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PEW_CORE_TRACE(...) ::PewPew::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PEW_CORE_CRITICAL(...) ::PewPew::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define PEW_ERROR(...) ::PewPew::Log::GetClientLogger()->error(__VA_ARGS__)
#define PEW_WARN(...)  ::PewPew::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PEW_INFO(...)  ::PewPew::Log::GetClientLogger()->info(__VA_ARGS__)
#define PEW_TRACE(...) ::PewPew::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PEW_CRITICAL(...) ::PewPew::Log::GetClientLogger()->critical(__VA_ARGS__)
