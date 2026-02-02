#pragma once
#include "PewPew/Core/Layer.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ProfilerPanel.h"

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

    private:
        // Panels
        SceneHierarchyPanel m_SceneHierarchyPanel;
        PropertiesPanel m_PropertiesPanel;
        ViewportPanel m_ViewportPanel;
        ConsolePanel m_ConsolePanel;
        ProfilerPanel m_ProfilerPanel;
    };
}
