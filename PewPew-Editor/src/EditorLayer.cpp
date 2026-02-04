#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "PewPew/Debug/Instrumentor.h"
#include "PewPew/Asset/AssetManager.h"
#include "PewPew/Scene/SceneManager.h"
#include "PewPew/Core/Application.h"
#include "PewPew/Utils/FileDialogs.h"

namespace PewPew
{
    EditorLayer::EditorLayer()
        : Layer("EditorLayer"), m_SceneHierarchyPanel(SceneManager::GetActiveScene()) { PEW_PROFILE_FUNCTION(); }

    void EditorLayer::OnAttach()
    {
        PEW_PROFILE_FUNCTION();

        // Initialize SceneManager with default empty scene
        SceneManager::Init();

        // Start file watcher for hot reload
        m_FileWatcher = CreateScope<FileWatcher>("assets", [](const FileWatcherEvent& event)
        {
            if (event.Action == FileAction::Modified)
            {
                // Get relative path
                std::filesystem::path relativePath = std::filesystem::relative(event.FilePath, "assets");
                UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

                if (uuid.IsValid())
                {
                    AssetManager::QueueReload(uuid);
                    PEW_CORE_INFO("File changed, queued for reload: {0}", event.FilePath.string());
                }
            }
        });
        m_FileWatcher->Start();
    }

    void EditorLayer::OnDetach()
    {
        PEW_PROFILE_FUNCTION();

        if (m_FileWatcher)
        {
            m_FileWatcher->Stop();
        }

        SceneManager::Shutdown();
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        PEW_PROFILE_FUNCTION();

        // Update viewport (handles rendering)
        m_ViewportPanel.OnUpdate(ts);
    }

    void EditorLayer::OnImGuiRender()
    {
        PEW_PROFILE_FUNCTION();

        BeginDockspace();

        DrawMenuBar();

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
            }
        }

        // Render all panels
        m_SceneHierarchyPanel.OnImGuiRender();
        m_PropertiesPanel.OnImGuiRender();
        m_ViewportPanel.OnImGuiRender();
        m_ConsolePanel.OnImGuiRender();
        m_ProfilerPanel.OnImGuiRender();
        m_ContentBrowserPanel.OnImGuiRender();

        EndDockspace();
    }

    void EditorLayer::OnEvent(Event& e)
    {
        PEW_PROFILE_FUNCTION();

        m_ProfilerPanel.OnEvent(e);
        m_ViewportPanel.OnEvent(e);
        m_ContentBrowserPanel.OnEvent(e);
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
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Scene Hierarchy", nullptr, m_SceneHierarchyPanel.GetVisiblePtr());
                ImGui::MenuItem("Properties", nullptr, m_PropertiesPanel.GetVisiblePtr());
                ImGui::MenuItem("Viewport", nullptr, m_ViewportPanel.GetVisiblePtr());
                ImGui::MenuItem("Console", nullptr, m_ConsolePanel.GetVisiblePtr());
                ImGui::MenuItem("Content Browser", nullptr, m_ContentBrowserPanel.GetVisiblePtr());
                ImGui::Separator();
                if (ImGui::MenuItem("Reset to Default Layout"))
                    m_ResetLayout = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools"))
            {
                ImGui::MenuItem("Profiler", "F3", m_ProfilerPanel.GetVisiblePtr());
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
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            // Setup default layout on first frame or when reset is requested
            if (m_FirstFrame || m_ResetLayout)
            {
                m_FirstFrame = false;
                m_ResetLayout = false;
                SetupDefaultLayout(dockspace_id);
            }
        }

        style.WindowMinSize.x = minWinSizeX;
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
        String filepath = FileDialogs::OpenFile("PewPew Scene (*.scene)\0*.scene\0All Files (*.*)\0*.*\0");
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
        String filepath = FileDialogs::SaveFile("PewPew Scene (*.scene)\0*.scene\0All Files (*.*)\0*.*\0");
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
        ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

        // Split the remaining area: left for Scene Hierarchy
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.18f, nullptr, &dock_main_id);

        // Split the remaining area: right for Properties
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);

        // The remaining dock_main_id is the center area for Viewport

        // Dock windows to their designated areas
        ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left_id);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Properties", dock_right_id);

        // Dock Content Browser and Console to the bottom (they will be tabbed)
        ImGui::DockBuilderDockWindow("Content Browser", dock_bottom_id);
        ImGui::DockBuilderDockWindow("Console", dock_bottom_id);

        // Finish building the layout
        ImGui::DockBuilderFinish(dockspace_id);
    }
}
