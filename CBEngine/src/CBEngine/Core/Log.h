#pragma once

#include "spdlog/logger.h"

#include "Core.h"

namespace CB
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

#define CB_CORE_ERROR(...) ::CB::Log::GetCoreLogger()->error(__VA_ARGS__)
#define CB_CORE_WARN(...)  ::CB::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CB_CORE_INFO(...)  ::CB::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CB_CORE_TRACE(...) ::CB::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CB_CORE_CRITICAL(...) ::CB::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define CB_ERROR(...) ::CB::Log::GetClientLogger()->error(__VA_ARGS__)
#define CB_WARN(...)  ::CB::Log::GetClientLogger()->warn(__VA_ARGS__)
#define CB_INFO(...)  ::CB::Log::GetClientLogger()->info(__VA_ARGS__)
#define CB_TRACE(...) ::CB::Log::GetClientLogger()->trace(__VA_ARGS__)
#define CB_CRITICAL(...) ::CB::Log::GetClientLogger()->critical(__VA_ARGS__)