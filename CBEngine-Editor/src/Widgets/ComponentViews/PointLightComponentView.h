#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/PointLightComponent.h"
#include "../ComponentCard.h"

namespace CB
{
	class PointLightComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<PointLightComponent>())
				return;

			bool removed = false;
			bool reset = false;

			bool opened = ComponentCard::Begin("Point Light", true, &removed, &reset);

			if (removed) {
				entity.RemoveComponent<PointLightComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) {
				auto& light = entity.GetComponent<PointLightComponent>();
				light.Color = Vector3(1.0f);
				light.Intensity = 1.0f;
				light.Range = 10.0f;
				light.Visible = true;
			}

			if (opened) {
				auto& light = entity.GetComponent<PointLightComponent>();

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
			}

			ComponentCard::End();
		}
	};
}
