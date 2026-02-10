#pragma once

#include "Scene.h"
#include "CBEngine/Core/Core.h"

namespace CB
{
    class SceneManager
    {
    public:
        static void Init();
        static void Shutdown();

        // Scene management
        static Ref<Scene> NewScene(const String& name = "Untitled");
        static Ref<Scene> LoadScene(const String& filepath);
        static bool SaveScene(const String& filepath);
        static bool SaveSceneAs(const String& filepath);

        // Active scene
        static Ref<Scene> GetActiveScene() { return s_ActiveScene; }
        static void SetActiveScene(const Ref<Scene>& scene);

        // Scene state
        static bool HasActiveScene() { return s_ActiveScene != nullptr; }
        static bool IsSceneModified() { return s_SceneModified; }
        static void MarkSceneModified() { s_SceneModified = true; }
        static const String& GetActiveScenePath() { return s_ActiveScenePath; }
        static const String& GetActiveSceneName() { return s_ActiveSceneName; }

        // Callbacks for when scene changes (optional, panels can use this)
        using SceneChangedCallback = std::function<void(const Ref<Scene>&)>;
        static void AddSceneChangedCallback(SceneChangedCallback callback);
        static void ClearCallbacks();

    private:
        static void NotifySceneChanged();

        static Ref<Scene> s_ActiveScene;
        static String s_ActiveScenePath;
        static String s_ActiveSceneName;
        static bool s_SceneModified;
        static std::vector<SceneChangedCallback> s_Callbacks;
    };
}
