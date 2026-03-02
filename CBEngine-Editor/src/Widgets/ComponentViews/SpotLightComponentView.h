#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/SpotLightComponent.h"
#include "../ComponentCard.h"

namespace CB
{
	class SpotLightComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<SpotLightComponent>())
				return;

			bool removed = false;
			bool reset = false;

			bool opened = ComponentCard::Begin("Spot Light", true, &removed, &reset);

			if (removed) {
				entity.RemoveComponent<SpotLightComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) {
				auto& light = entity.GetComponent<SpotLightComponent>();
				light.Color = Vector3(1.0f);
				light.Intensity = 1.0f;
				light.Range = 10.0f;
				light.InnerAngleDegrees = 25.0f;
				light.OuterAngleDegrees = 35.0f;
				light.Visible = true;
			}

			if (opened) {
				auto& light = entity.GetComponent<SpotLightComponent>();

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
				ImGui::DragFloat("Range", &light.Range, 0.1f, 0.1f, 1000.0f, "%.1f");

				ImGui::Spacing();
				ImGui::TextUnformatted("Cone");
				ImGui::Separator();

				ImGui::DragFloat("Inner Angle (deg)", &light.InnerAngleDegrees, 0.5f, 0.0f, 89.0f, "%.1f");
				ImGui::DragFloat("Outer Angle (deg)", &light.OuterAngleDegrees, 0.5f, 0.0f, 90.0f, "%.1f");

				// Clamp inner <= outer
				if (light.InnerAngleDegrees > light.OuterAngleDegrees)
					light.InnerAngleDegrees = light.OuterAngleDegrees;
			}

			ComponentCard::End();
		}
	};
}
