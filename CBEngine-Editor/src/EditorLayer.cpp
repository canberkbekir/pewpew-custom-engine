#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Core/Application.h"
#include "CBEngine/Debug/ColliderDebugRenderer.h"
#include "CBEngine/Utils/FileDialogs.h"
#include "CBEngine/Audio/AudioEngine.h"
#include "CBEngine/Renderer/Resources/PrimitiveMesh.h"

namespace CB
{
    // Static member definitions
    std::queue<EditorLayer::ImportRequest> EditorLayer::s_PendingImports;
    std::mutex EditorLayer::s_PendingImportsMutex;
    std::queue<EditorLayer::ReimportRequest> EditorLayer::s_PendingReimports;
    std::queue<UUID> EditorLayer::s_PendingVtexOpens;

    EditorLayer::EditorLayer()
        : Layer("EditorLayer"), m_SceneHierarchyPanel(SceneManager::GetActiveScene()) { CB_PROFILE_FUNCTION(); }

    void EditorLayer::OnAttach()
    {
        CB_PROFILE_FUNCTION();

        // Initialize SceneManager with default empty scene
        SceneManager::Init();

        // Start file watcher for hot reload
        m_FileWatcher = CreateScope<FileWatcher>("assets", [](const FileWatcherEvent& event)
        {
            if (event.Action == FileAction::Modified)
            {
                // Get relative path
                std::filesystem::path relativePath = relative(event.FilePath, "assets");
                UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

                if (uuid.IsValid())
                {
                    AssetManager::QueueReload(uuid);
                    CB_CORE_INFO("File changed, queued for reload: {0}", event.FilePath.string());
                }
            }
            else if (event.Action == FileAction::Added)
            {
                // Check if it's a raw mesh file — queue for import preview
                std::string ext = event.FilePath.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
                if (IsRawMeshExtension(ext))
                {
                    // Output .mesh/.vmesh next to the source file
                    RequestImportPreview(event.FilePath, event.FilePath.parent_path());
                }
            }
        });
        m_FileWatcher->Start();

        // Scan for GameManager-derived scripts
        ScriptEngine::ScanForGameManagerScripts();

        // Initialize audio engine
        AudioEngine::Init();

        // Pre-generate built-in primitive meshes and register them in the AssetManager.
        // Must be done here (on the GL thread, after AssetManager is ready).
        PrimitiveMesh::InitAll();
    }

    void EditorLayer::OnDetach()
    {
        CB_PROFILE_FUNCTION();

        if (m_FileWatcher)
        {
            m_FileWatcher->Stop();
        }

        ColliderDebugRenderer::Shutdown();
        AudioEngine::Shutdown();
        SceneManager::Shutdown();
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        CB_PROFILE_FUNCTION();

        // Process pending import requests from FileWatcher
        {
            std::lock_guard<std::mutex> lock(s_PendingImportsMutex);
            while (!s_PendingImports.empty())
            {
                auto request = s_PendingImports.front();
                s_PendingImports.pop();
                m_MeshImportPanel.OpenForFile(request.SourcePath, request.OutputDirectory);
            }
            while (!s_PendingReimports.empty())
            {
                auto request = s_PendingReimports.front();
                s_PendingReimports.pop();
                m_MeshImportPanel.OpenForReimport(request.AssetUUID);
            }
            while (!s_PendingVtexOpens.empty())
            {
                UUID vtexUUID = s_PendingVtexOpens.front();
                s_PendingVtexOpens.pop();
                m_VoxelTextureEditorPanel.OpenForAsset(vtexUUID);
            }
        }

        // Process hot reload queue
        AssetManager::ProcessReloadQueue();

        // Update scene (transform hierarchy, physics, scripts, etc.)
        Ref<Scene> scene = SceneManager::GetActiveScene();
        if (scene)
            scene->OnUpdate(ts);

        // Consume any deferred scene change requested by scripts
        if (ScriptEngine::HasPendingSceneChange())
        {
            String pendingPath = ScriptEngine::GetPendingScenePath();
            ScriptEngine::ClearPendingSceneChange();
            if (pendingPath.empty())
            {
                // Reload current scene
                String currentPath = SceneManager::GetActiveScenePath();
                if (!currentPath.empty())
                    SceneManager::LoadScene(currentPath);
            }
            else
            {
                SceneManager::LoadScene(pendingPath);
            }
        }

        // Update audio engine listener using active scene camera (if any)
        AudioEngine::Update(Vector3(0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f));

        // Update viewport (handles rendering)
        m_ViewportPanel.OnUpdate(ts);

        // Update game viewport
        m_GameViewportPanel.OnUpdate(ts);

        // Update import preview panel
        m_MeshImportPanel.OnUpdate(ts);

        // Update voxel texture editor panel
        m_VoxelTextureEditorPanel.OnUpdate(ts);
    }

    void EditorLayer::OnImGuiRender()
    {
        CB_PROFILE_FUNCTION();

        BeginDockspace();

        DrawMenuBar();

        // Create DockSpace below the menu bar
        {
            ImGuiIO& io = ImGui::GetIO();
            ImGuiStyle& style = ImGui::GetStyle();
            float minWinSizeX = style.WindowMinSize.x;
            style.WindowMinSize.x = 370.0f;

            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

                if (m_FirstFrame || m_ResetLayout)
                {
                    m_FirstFrame = false;
                    m_ResetLayout = false;
                    SetupDefaultLayout(dockspace_id);
                }
            }

            style.WindowMinSize.x = minWinSizeX;
        }

        // Global keyboard shortcuts
        {
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantTextInput)
            {
                if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
                    SaveSceneAs();
                else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
                    SaveScene();
                else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
                    NewScene();
                else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
                    OpenScene();
                else if (ImGui::IsKeyPressed(ImGuiKey_F5))
                {
                    Ref<Scene> scene = SceneManager::GetActiveScene();
                    if (scene)
                    {
                        if (scene->IsPhysicsInitialized())
                            scene->ShutdownPhysics();
                        else
                            scene->InitPhysics();
                    }
                }
            }
        }

        // Render all panels
        m_SceneHierarchyPanel.OnImGuiRender();
        m_PropertiesPanel.OnImGuiRender();
        m_ViewportPanel.OnImGuiRender();
        m_ConsolePanel.OnImGuiRender();
        m_ProfilerPanel.OnImGuiRender();
        m_ContentBrowserPanel.OnImGuiRender();
        m_MeshImportPanel.OnImGuiRender();
        m_VoxelTextureEditorPanel.OnImGuiRender();
        m_GameViewportPanel.OnImGuiRender();
        m_SubstanceEditorPanel.OnImGuiRender();

        EndDockspace();
    }

    void EditorLayer::OnEvent(Event& e)
    {
        CB_PROFILE_FUNCTION();

        m_ProfilerPanel.OnEvent(e);
        if (e.Handled) return;

        m_MeshImportPanel.OnEvent(e);
        if (e.Handled) return;

        m_VoxelTextureEditorPanel.OnEvent(e);
        if (e.Handled) return;

        m_ViewportPanel.OnEvent(e);
        if (e.Handled) return;

        m_ContentBrowserPanel.OnEvent(e);
    }

    void EditorLayer::RequestImportPreview(const std::filesystem::path& path, const std::filesystem::path& outputDir)
    {
        std::lock_guard<std::mutex> lock(s_PendingImportsMutex);
        s_PendingImports.push({path, outputDir});
    }

    void EditorLayer::RequestImportPreviewReimport(const std::filesystem::path& path, UUID uuid)
    {
        std::lock_guard<std::mutex> lock(s_PendingImportsMutex);
        s_PendingReimports.push({path, uuid});
    }

    void EditorLayer::RequestOpenVoxelTexture(UUID vtexUUID)
    {
        std::lock_guard<std::mutex> lock(s_PendingImportsMutex);
        s_PendingVtexOpens.push(vtexUUID);
    }

    void EditorLayer::DrawMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    NewScene();
                }
                if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
                {
                    OpenScene();
                }
                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                {
                    SaveScene();
                }
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                {
                    SaveSceneAs();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    Application::Get().Close();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z"))
                {
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y"))
                {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Scene Hierarchy", nullptr, m_SceneHierarchyPanel.GetVisiblePtr());
                ImGui::MenuItem("Properties", nullptr, m_PropertiesPanel.GetVisiblePtr());
                ImGui::MenuItem("Viewport", nullptr, m_ViewportPanel.GetVisiblePtr());
                ImGui::MenuItem("Game", nullptr, m_GameViewportPanel.GetVisiblePtr());
                ImGui::MenuItem("Console", nullptr, m_ConsolePanel.GetVisiblePtr());
                ImGui::MenuItem("Content Browser", nullptr, m_ContentBrowserPanel.GetVisiblePtr());
                ImGui::Separator();
                if (ImGui::MenuItem("Reset to Default Layout"))
                    m_ResetLayout = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Physics"))
            {
                Ref<Scene> scene = SceneManager::GetActiveScene();
                if (scene)
                {
                    bool simulating = scene->IsPhysicsInitialized();
                    if (ImGui::MenuItem(simulating ? "Stop Simulation" : "Start Simulation", "F5"))
                    {
                        if (simulating)
                            scene->ShutdownPhysics();
                        else
                            scene->InitPhysics();
                    }
                }
                else
                {
                    ImGui::TextDisabled("No active scene");
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools"))
            {
                ImGui::MenuItem("Profiler", "F3", m_ProfilerPanel.GetVisiblePtr());
                ImGui::MenuItem("Mesh Import", nullptr, m_MeshImportPanel.GetVisiblePtr());
                ImGui::MenuItem("Voxel Texture Editor", nullptr, m_VoxelTextureEditorPanel.GetVisiblePtr());
                ImGui::MenuItem("Substance Editor", nullptr, m_SubstanceEditorPanel.GetVisiblePtr());
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void EditorLayer::BeginDockspace()
    {
        static bool dockspaceOpen = true;
        static bool opt_fullscreen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

    }

    void EditorLayer::EndDockspace()
    {
        ImGui::End();
    }

    void EditorLayer::NewScene()
    {
        SceneManager::NewScene();
    }

    void EditorLayer::OpenScene()
    {
        String filepath = FileDialogs::OpenFile("CBEngine Scene (*.scene)\0*.scene\0All Files (*.*)\0*.*\0");
        if (!filepath.empty())
        {
            SceneManager::LoadScene(filepath);
        }
    }

    void EditorLayer::SaveScene()
    {
        if (!SceneManager::GetActiveScenePath().empty())
        {
            SceneManager::SaveScene(SceneManager::GetActiveScenePath());
        }
        else
        {
            SaveSceneAs();
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        String filepath = FileDialogs::SaveFile("CBEngine Scene (*.scene)\0*.scene\0All Files (*.*)\0*.*\0");
        if (!filepath.empty())
        {
            SceneManager::SaveSceneAs(filepath);
        }
    }

    void EditorLayer::SetupDefaultLayout(ImGuiID dockspace_id)
    {
        // Clear any existing layout
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

        // Split the dockspace into regions
        // First, split off the bottom area for Content Browser and Console
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_bottom_id =
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

        // Split the remaining area: left for Scene Hierarchy
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.18f, nullptr, &dock_main_id);

        // Split the remaining area: right for Properties
        ImGuiID dock_right_id =
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);

        // The remaining dock_main_id is the center area for Viewport

        // Dock windows to their designated areas
        ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left_id);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Game", dock_main_id);
        ImGui::DockBuilderDockWindow("Properties", dock_right_id);

        // Dock Content Browser and Console to the bottom (they will be tabbed)
        ImGui::DockBuilderDockWindow("Content Browser", dock_bottom_id);
        ImGui::DockBuilderDockWindow("Console", dock_bottom_id);

        // Finish building the layout
        ImGui::DockBuilderFinish(dockspace_id);
    }
}
