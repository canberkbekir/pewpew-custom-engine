#pragma once

#include "Plugin.h"
#include "CBEngine/Core/Core.h"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace CB
{
	class CB_API PluginLoader
	{
	public:
		static bool LoadPlugin(const std::filesystem::path& dllPath);
		static void UnloadPlugin(const char* name);
		static void UnloadAll();

		static IPlugin* GetPlugin(const char* name);

	private:
		struct LoadedPlugin
		{
			IPlugin* Instance = nullptr;
			void* DllHandle = nullptr; // HMODULE on Windows
		};

		static std::unordered_map<std::string, LoadedPlugin> s_LoadedPlugins;
	};
}
