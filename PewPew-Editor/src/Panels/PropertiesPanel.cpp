#include "PropertiesPanel.h"
#include "imgui.h"

namespace PewPew
{
    void PropertiesPanel::OnImGuiRender()
    {
        if (!m_Visible)
            return;

        ImGui::Begin(m_Name.c_str(), &m_Visible);

        // TODO: Draw component properties when entity is selected
        ImGui::Text("Select an entity to view properties");

        ImGui::End();
    }
}
