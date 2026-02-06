#pragma once
#include "CBEngine/Core/Layer.h"
#include "CBEngine/FileWatcher/FileWatcher.h"

typedef unsigned int ImGuiID;

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ProfilerPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ImportPreviewPanel.h"

#include <queue>
#include <mutex>
#include <filesystem>

namespace CB
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

        // Request import preview for a file (thread-safe, called from FileWatcher or ContentBrowser)
        static void RequestImportPreview(const std::filesystem::path& path);
        static void RequestImportPreviewReimport(const std::filesystem::path& path, UUID uuid);

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
        ImportPreviewPanel m_ImportPreviewPanel;

        // Pending import queue (thread-safe, for FileWatcher callback)
        static std::queue<std::filesystem::path> s_PendingImports;
        static std::mutex s_PendingImportsMutex;

        // Pending reimport queue
        struct ReimportRequest { std::filesystem::path Path; UUID AssetUUID; };
        static std::queue<ReimportRequest> s_PendingReimports;
    };
}
