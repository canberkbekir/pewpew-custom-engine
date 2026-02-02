#include "EditorLayer.h"

#include "imgui.h"
#include "PewPew/Debug/Instrumentor.h"

namespace PewPew
{
    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    {
        PEW_PROFILE_FUNCTION();
    }

    void EditorLayer::OnAttach()
    {
        PEW_PROFILE_FUNCTION();
    }

    void EditorLayer::OnDetach()
    {
        PEW_PROFILE_FUNCTION();
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

        // Render all panels
        m_SceneHierarchyPanel.OnImGuiRender();
        m_PropertiesPanel.OnImGuiRender();
        m_ViewportPanel.OnImGuiRender();
        m_ConsolePanel.OnImGuiRender();
        m_ProfilerPanel.OnImGuiRender();

        EndDockspace();
    }

    void EditorLayer::OnEvent(Event& e)
    {
        PEW_PROFILE_FUNCTION();

        m_ProfilerPanel.OnEvent(e);
        m_ViewportPanel.OnEvent(e);
    }

    void EditorLayer::DrawMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {}
                if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {}
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {}
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {}
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
        }

        style.WindowMinSize.x = minWinSizeX;
    }

    void EditorLayer::EndDockspace()
    {
        ImGui::End();
    }
}
