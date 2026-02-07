#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/DirectionalLightComponent.h"

namespace CB
{
	class DirectionalLightComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<DirectionalLightComponent>())
				return;

			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_AllowItemOverlap |
				ImGuiTreeNodeFlags_SpanAvailWidth;

			bool opened = ImGui::TreeNodeEx("Directional Light", flags);

			// Right-aligned remove button
			{
				float lineHeight = ImGui::GetFrameHeight();
				float rightX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - lineHeight;
				ImGui::SameLine(rightX);
				if (ImGui::Button("X##DirectionalLight", ImVec2(lineHeight, lineHeight)))
				{
					entity.RemoveComponent<DirectionalLightComponent>();
					if (opened) ImGui::TreePop();
					return;
				}
			}

			if (opened)
			{
				auto& light = entity.GetComponent<DirectionalLightComponent>();

				ImGui::Checkbox("Visible", &light.Visible);

				ImGui::Spacing();
				ImGui::TextUnformatted("Light");
				ImGui::Separator();

				ImGui::ColorButton("##ColorPreview",
					ImVec4(light.Color.x, light.Color.y, light.Color.z, 1.0f),
					ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
					ImVec2(16, 16));
				ImGui::SameLine();

				ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&light.Color),
					ImGuiColorEditFlags_NoTooltip);

				ImGui::DragFloat("Intensity", &light.Intensity, 0.05f, 0.0f, 100.0f, "%.2f");

				ImGui::Spacing();
				ImGui::TextUnformatted("Shadows");
				ImGui::Separator();

				ImGui::Checkbox("Cast Shadows", &light.CastShadows);

				ImGui::BeginDisabled(!light.CastShadows);
				ImGui::DragFloat("Source Angle (deg)", &light.SourceAngleDegrees, 0.01f, 0.0f, 10.0f, "%.2f");
				ImGui::EndDisabled();

				ImGui::TextDisabled("Tip: Sun is ~0.53 deg");

				ImGui::TreePop();
			}
		}
	};
}
