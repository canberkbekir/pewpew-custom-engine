#pragma once
#include "PewPew/Core/Layer.h"
#include "PewPew/FileWatcher/FileWatcher.h"

typedef unsigned int ImGuiID;

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ProfilerPanel.h"
#include "Panels/ContentBrowserPanel.h"

namespace PewPew
{
    class EditorLayer : public Layer
    {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(Timestep ts) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        void BeginDockspace();
        void EndDockspace();
        void DrawMenuBar();
        void SetupDefaultLayout(ImGuiID dockspace_id);

        // Scene file operations
        void NewScene();
        void OpenScene();
        void SaveScene();
        void SaveSceneAs();

    private:
        // File Watcher for hot reload
        Scope<FileWatcher> m_FileWatcher;

        // Layout
        bool m_ResetLayout = false;
        bool m_FirstFrame = true;

        // Panels
        SceneHierarchyPanel m_SceneHierarchyPanel;
        PropertiesPanel m_PropertiesPanel;
        ViewportPanel m_ViewportPanel;
        ConsolePanel m_ConsolePanel;
        ProfilerPanel m_ProfilerPanel;
        ContentBrowserPanel m_ContentBrowserPanel;
    };
}
