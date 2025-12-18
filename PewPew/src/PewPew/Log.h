#pragma once
#include "Core.h"
#include "spdlog/spdlog.h" 
namespace PewPew
{
	class PEW_API Log
	{
	public:
		static void Init();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define PEW_CORE_ERROR(...) ::PewPew::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PEW_CORE_WARN(...)  ::PewPew::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PEW_CORE_INFO(...)  ::PewPew::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PEW_CORE_TRACE(...) ::PewPew::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PEW_CORE_FATAL(...) ::PewPew::Log::GetCoreLogger()->fatal(__VA_ARGS__)

#define PEW_ERROR(...) ::PewPew::Log::GetClientLogger()->error(__VA_ARGS__)
#define PEW_WARN(...)  ::PewPew::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PEW_INFO(...)  ::PewPew::Log::GetClientLogger()->info(__VA_ARGS__)
#define PEW_TRACE(...) ::PewPew::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PEW_FATAL(...) ::PewPew::Log::GetClientLogger()->fatal(__VA_ARGS__)