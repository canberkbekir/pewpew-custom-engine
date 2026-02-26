#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/CameraComponent.h"
#include "../ComponentCard.h"

namespace CB
{
	class CameraComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<CameraComponent>())
				return;

			bool removed = false;
			bool reset = false;

			bool opened = ComponentCard::Begin("Camera", true, &removed, &reset);

			if (removed) {
				entity.RemoveComponent<CameraComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) {
				auto& cam = entity.GetComponent<CameraComponent>();
				cam.Primary = false;
				cam.FOV = 60.0f;
				cam.NearClip = 0.1f;
				cam.FarClip = 1000.0f;
			}

			if (opened) {
				auto& camera = entity.GetComponent<CameraComponent>();

				ImGui::Checkbox("Primary", &camera.Primary);

				ImGui::DragFloat("FOV", &camera.FOV, 0.5f, 1.0f, 179.0f, "%.1f");
				ImGui::DragFloat("Near Clip", &camera.NearClip, 0.01f, 0.001f, 10.0f, "%.3f");
				ImGui::DragFloat("Far Clip", &camera.FarClip, 1.0f, 1.0f, 100000.0f, "%.0f");
			}

			ComponentCard::End();
		}
	};
}