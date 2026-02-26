#include "cbpch.h"
#include "PluginLoader.h"
#include "CBEngine/Core/Application.h"

#ifdef CB_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace CB
{
	std::unordered_map<std::string, PluginLoader::LoadedPlugin> PluginLoader::s_LoadedPlugins;

	bool PluginLoader::LoadPlugin(const std::filesystem::path& dllPath)
	{
#ifdef CB_PLATFORM_WINDOWS
		HMODULE hModule = LoadLibraryA(dllPath.string().c_str());
		if (!hModule) {
			CB_CORE_ERROR("Failed to load plugin DLL: {0} (error: {1})", dllPath.string(), GetLastError());
			return false;
		}

		using CreatePluginFn = IPlugin* (*)();
		auto createPlugin = reinterpret_cast<CreatePluginFn>(GetProcAddress(hModule, "CreatePlugin"));
		if (!createPlugin) {
			CB_CORE_ERROR("Plugin DLL missing CreatePlugin export: {0}", dllPath.string());
			FreeLibrary(hModule);
			return false;
		}

		IPlugin* plugin = createPlugin();
		if (!plugin) {
			CB_CORE_ERROR("CreatePlugin returned nullptr: {0}", dllPath.string());
			FreeLibrary(hModule);
			return false;
		}

		const char* name = plugin->GetName();
		CB_CORE_INFO("Loading plugin: {0}", name);

		// Unload existing plugin with same name
		UnloadPlugin(name);

		plugin->OnLoad(Application::Get());

		LoadedPlugin lp;
		lp.Instance = plugin;
		lp.DllHandle = hModule;
		s_LoadedPlugins[name] = lp;

		CB_CORE_INFO("Plugin loaded: {0}", name);
		return true;
#else
		CB_CORE_ERROR("Plugin loading not supported on this platform");
		return false;
#endif
	}

	void PluginLoader::UnloadPlugin(const char* name)
	{
		auto it = s_LoadedPlugins.find(name);
		if (it == s_LoadedPlugins.end())
			return;

		CB_CORE_INFO("Unloading plugin: {0}", name);

		it->second.Instance->OnUnload();
		delete it->second.Instance;

#ifdef CB_PLATFORM_WINDOWS
		if (it->second.DllHandle)
			FreeLibrary(static_cast<HMODULE>(it->second.DllHandle));
#endif

		s_LoadedPlugins.erase(it);
	}

	void PluginLoader::UnloadAll()
	{
		// Collect names first to avoid iterator invalidation
		std::vector<std::string> names;
		names.reserve(s_LoadedPlugins.size());
		for (auto& [name, _] : s_LoadedPlugins)
			names.push_back(name);

		for (auto& name : names)
			UnloadPlugin(name.c_str());
	}

	IPlugin* PluginLoader::GetPlugin(const char* name)
	{
		auto it = s_LoadedPlugins.find(name);
		if (it != s_LoadedPlugins.end())
			return it->second.Instance;
		return nullptr;
	}
}