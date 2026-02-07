#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/CoreComponents.h"

#include <cstring>

namespace CB
{
	class TagComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<TagComponent>())
				return;

			auto& tag = entity.GetComponent<TagComponent>();

			char buffer[256];
			strncpy_s(buffer, tag.Tag.c_str(), sizeof(buffer) - 1);

			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1);
			if (ImGui::InputText("##EntityName", buffer, sizeof(buffer)))
			{
				tag.Tag = buffer;
			}

			// Show UUID
			if (entity.HasComponent<IDComponent>())
			{
				UUID uuid = entity.GetComponent<IDComponent>().ID;
				ImGui::TextDisabled("UUID: %llu", static_cast<uint64_t>(uuid));
			}
		}
	};
}
