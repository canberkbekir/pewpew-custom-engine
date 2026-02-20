#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/Scene.h" 
#include "CBEngine/Core/Application.h"
#include "CBEngine/Debug/ColliderDebugRenderer.h"
#include "CBEngine/Utils/FileDialogs.h"

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

        // Load toolbar icons
        m_PlayButtonIcon = Texture2D::Create("resources/icons/play_btn.png");
        m_PauseButtonIcon = Texture2D::Create("resources/icons/pause_btn.png");
        m_StopButtonIcon = Texture2D::Create("resources/icons/stop_btn.png");
        m_NextFrameButtonIcon = Texture2D::Create("resources/icons/next_frame_btn.png");
        m_ResumeButtonIcon = Texture2D::Create("resources/icons/resume_btn.png");

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
    }

    void EditorLayer::OnDetach()
    {
        CB_PROFILE_FUNCTION();

        if (m_FileWatcher)
        {
            m_FileWatcher->Stop();
        }

        ColliderDebugRenderer::Shutdown();
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

        // Update scene (transform hierarchy, physics, etc.)
        Ref<Scene> scene = SceneManager::GetActiveScene();
        if (scene)
            scene->OnUpdate(ts);

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
        DrawToolbar();

        // Create DockSpace after toolbar so it takes remaining space below
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
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void EditorLayer::DrawToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.305f, 0.31f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.1505f, 0.151f, 0.5f));

        float toolbarHeight = 32.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));

        ImGui::BeginChild("##EditorToolbar", ImVec2(0, toolbarHeight), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        Ref<Scene> scene = SceneManager::GetActiveScene();
        bool simulating = scene && scene->IsPhysicsInitialized();
        bool paused = scene && scene->IsPhysicsPaused();

        // Calculate total width of buttons to center them
        float buttonSize = 24.0f;
        float spacing = 4.0f;
        float totalWidth = buttonSize; // Play or Stop
        if (simulating)
            totalWidth += spacing + buttonSize + spacing + buttonSize; // + Pause/Resume + Step

        float availWidth = ImGui::GetContentRegionAvail().x;
        float startX = (availWidth - totalWidth) * 0.5f;
        if (startX < 0) startX = 0;

        ImGui::SetCursorPosX(startX);
        ImGui::SetCursorPosY((toolbarHeight - buttonSize) * 0.5f);

        if (!simulating)
        {
            if (ImGui::ImageButton(
                "##play",
                (ImTextureID)(uintptr_t)m_PlayButtonIcon->GetRendererID(),
                ImVec2(buttonSize, buttonSize),
                ImVec2(0, 1), ImVec2(1, 0)))
            {
                if (scene)
                    scene->InitPhysics();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play (F5)");
        }
        else
        {
            // Stop button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 0.3f));
            if (ImGui::ImageButton(
                "##stop",
                (ImTextureID)(uintptr_t)m_StopButtonIcon->GetRendererID(),
                ImVec2(buttonSize, buttonSize),
                ImVec2(0, 1), ImVec2(1, 0)))
            {
                scene->ShutdownPhysics();
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop (F5)");

            ImGui::SameLine(0.0f, spacing);

            // Pause / Resume
            if (paused)
            {
                if (ImGui::ImageButton(
                    "##resume",
                    (ImTextureID)(uintptr_t)m_ResumeButtonIcon->GetRendererID(),
                    ImVec2(buttonSize, buttonSize),
                    ImVec2(0, 1), ImVec2(1, 0)))
                {
                    scene->ResumePhysics();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resume");
            }
            else
            {
                if (ImGui::ImageButton(
                    "##pause",
                    (ImTextureID)(uintptr_t)m_PauseButtonIcon->GetRendererID(),
                    ImVec2(buttonSize, buttonSize),
                    ImVec2(0, 1), ImVec2(1, 0)))
                {
                    scene->PausePhysics();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause");
            }

            ImGui::SameLine(0.0f, spacing);

            // Step one frame
            bool canStep = paused;
            if (!canStep)
                ImGui::BeginDisabled();

            if (ImGui::ImageButton(
                "##step",
                (ImTextureID)(uintptr_t)m_NextFrameButtonIcon->GetRendererID(),
                ImVec2(buttonSize, buttonSize),
                ImVec2(0, 1), ImVec2(1, 0)))
            {
                if (canStep)
                    scene->StepOneFrame();
            }

            if (!canStep)
                ImGui::EndDisabled();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Step One Frame (only when paused)");
        }

        ImGui::EndChild();
        ImGui::Separator();

        ImGui::PopStyleColor(4); // ChildBg, Button, ButtonHovered, ButtonActive
        ImGui::PopStyleVar(2);
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
