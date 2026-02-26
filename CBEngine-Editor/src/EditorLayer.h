#pragma once
#include "CBEngine/Core/Layer.h"
#include "CBEngine/FileWatcher/FileWatcher.h"
#include "CBEngine/Renderer/Resources/Texture.h"

using ImGuiID = unsigned int;

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ProfilerPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/MeshImportPanel.h"
#include "Panels/VoxelTextureEditorPanel.h"
#include "Panels/GameViewportPanel.h"
#include "Panels/SubstanceEditorPanel.h"

#include <queue>
#include <mutex>
#include <filesystem>

namespace CB
{
    class EditorLayer : public Layer
    {
    public:
        EditorLayer();
        ~EditorLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(Timestep ts) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

        // Request import preview for a file (thread-safe, called from FileWatcher or ContentBrowser)
        // outputDir: where to write .mesh/.vmesh files. Empty = use source file's parent directory.
        static void RequestImportPreview(const std::filesystem::path& path,
                                         const std::filesystem::path& outputDir = {});
        static void RequestImportPreviewReimport(const std::filesystem::path& path, UUID uuid);
        static void RequestOpenVoxelTexture(UUID vtexUUID);

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
        MeshImportPanel m_MeshImportPanel;
        VoxelTextureEditorPanel m_VoxelTextureEditorPanel;
        GameViewportPanel m_GameViewportPanel;
        SubstanceEditorPanel m_SubstanceEditorPanel;

        // Pending import queue (thread-safe, for FileWatcher callback)
        struct ImportRequest
        {
            std::filesystem::path SourcePath;
            std::filesystem::path OutputDirectory;
        };

        static std::queue<ImportRequest> s_PendingImports;
        static std::mutex s_PendingImportsMutex;

        // Pending reimport queue
        struct ReimportRequest
        {
            std::filesystem::path Path;
            UUID AssetUUID;
        };

        static std::queue<ReimportRequest> s_PendingReimports;

        // Pending vtex editor open requests
        static std::queue<UUID> s_PendingVtexOpens;
    };
}
