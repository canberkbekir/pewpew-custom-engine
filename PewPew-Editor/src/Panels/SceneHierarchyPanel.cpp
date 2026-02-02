#include "SceneHierarchyPanel.h"
#include "imgui.h"

namespace PewPew
{
	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::Begin(m_Name.c_str(), &m_Visible);

		// TODO: Draw entity tree when ECS is implemented
		ImGui::Text("No entities in scene");

		ImGui::End();
	}
}