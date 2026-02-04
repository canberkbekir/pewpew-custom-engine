#include "pewpch.h"
#include "SceneManager.h"
#include "PewPew/Core/Log.h"

namespace PewPew
{
	Ref<Scene> SceneManager::s_ActiveScene = nullptr;
	String SceneManager::s_ActiveScenePath = "";
	String SceneManager::s_ActiveSceneName = "Untitled";
	bool SceneManager::s_SceneModified = false;
	std::vector<SceneManager::SceneChangedCallback> SceneManager::s_Callbacks;

	void SceneManager::Init()
	{
		// Create default empty scene on startup
		NewScene("Untitled");
		PEW_CORE_INFO("SceneManager initialized with default scene");
	}

	void SceneManager::Shutdown()
	{
		s_ActiveScene = nullptr;
		s_Callbacks.clear();
		PEW_CORE_INFO("SceneManager shutdown");
	}

	Ref<Scene> SceneManager::NewScene(const String& name)
	{
		s_ActiveScene = CreateRef<Scene>();
		s_ActiveScenePath = "";
		s_ActiveSceneName = name;
		s_SceneModified = false;

		NotifySceneChanged();

		PEW_CORE_INFO("Created new scene: {0}", name);
		return s_ActiveScene;
	}

	Ref<Scene> SceneManager::LoadScene(const String& filepath)
	{
		// TODO: Implement scene serialization/deserialization
		// For now, create a new scene and set the path

		s_ActiveScene = CreateRef<Scene>();
		s_ActiveScenePath = filepath;

		// Extract filename without extension as scene name
		std::filesystem::path path(filepath);
		s_ActiveSceneName = path.stem().string();
		s_SceneModified = false;

		// TODO: Call SceneSerializer::Deserialize(s_ActiveScene, filepath);

		NotifySceneChanged();

		PEW_CORE_INFO("Loaded scene: {0}", filepath);
		return s_ActiveScene;
	}

	bool SceneManager::SaveScene(const String& filepath)
	{
		if (filepath.empty() && s_ActiveScenePath.empty())
		{
			PEW_CORE_WARN("No filepath specified for saving scene");
			return false;
		}

		String savePath = filepath.empty() ? s_ActiveScenePath : filepath;

		// TODO: Call SceneSerializer::Serialize(s_ActiveScene, savePath);

		s_ActiveScenePath = savePath;
		std::filesystem::path path(savePath);
		s_ActiveSceneName = path.stem().string();
		s_SceneModified = false;

		PEW_CORE_INFO("Saved scene: {0}", savePath);
		return true;
	}

	bool SceneManager::SaveSceneAs(const String& filepath)
	{
		return SaveScene(filepath);
	}

	void SceneManager::SetActiveScene(const Ref<Scene>& scene)
	{
		s_ActiveScene = scene;
		s_SceneModified = false;
		NotifySceneChanged();
	}

	void SceneManager::AddSceneChangedCallback(SceneChangedCallback callback)
	{
		s_Callbacks.push_back(callback);
	}

	void SceneManager::ClearCallbacks()
	{
		s_Callbacks.clear();
	}

	void SceneManager::NotifySceneChanged()
	{
		for (auto& callback : s_Callbacks)
		{
			callback(s_ActiveScene);
		}
	}
}
