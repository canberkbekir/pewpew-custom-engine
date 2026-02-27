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

// Category-tagged core log macros
#define CB_CORE_TRACE_PHYSICS(...)    CB_CORE_TRACE("[Physics] " __VA_ARGS__)
#define CB_CORE_INFO_PHYSICS(...)     CB_CORE_INFO("[Physics] " __VA_ARGS__)
#define CB_CORE_WARN_PHYSICS(...)     CB_CORE_WARN("[Physics] " __VA_ARGS__)
#define CB_CORE_ERROR_PHYSICS(...)    CB_CORE_ERROR("[Physics] " __VA_ARGS__)

#define CB_CORE_TRACE_VOXELS(...)     CB_CORE_TRACE("[Voxels] " __VA_ARGS__)
#define CB_CORE_INFO_VOXELS(...)      CB_CORE_INFO("[Voxels] " __VA_ARGS__)
#define CB_CORE_WARN_VOXELS(...)      CB_CORE_WARN("[Voxels] " __VA_ARGS__)
#define CB_CORE_ERROR_VOXELS(...)     CB_CORE_ERROR("[Voxels] " __VA_ARGS__)

#define CB_CORE_TRACE_SCRIPTING(...)  CB_CORE_TRACE("[Scripting] " __VA_ARGS__)
#define CB_CORE_INFO_SCRIPTING(...)   CB_CORE_INFO("[Scripting] " __VA_ARGS__)
#define CB_CORE_WARN_SCRIPTING(...)   CB_CORE_WARN("[Scripting] " __VA_ARGS__)
#define CB_CORE_ERROR_SCRIPTING(...)  CB_CORE_ERROR("[Scripting] " __VA_ARGS__)

#define CB_CORE_TRACE_RENDERING(...)  CB_CORE_TRACE("[Rendering] " __VA_ARGS__)
#define CB_CORE_INFO_RENDERING(...)   CB_CORE_INFO("[Rendering] " __VA_ARGS__)
#define CB_CORE_WARN_RENDERING(...)   CB_CORE_WARN("[Rendering] " __VA_ARGS__)
#define CB_CORE_ERROR_RENDERING(...)  CB_CORE_ERROR("[Rendering] " __VA_ARGS__)