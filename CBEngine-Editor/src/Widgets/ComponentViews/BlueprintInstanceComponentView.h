#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/BlueprintInstanceComponent.h"
#include "../ComponentCard.h"

namespace CB
{
	class BlueprintInstanceComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<BlueprintInstanceComponent>())
				return;

			bool opened = ComponentCard::Begin("Blueprint Instance (Internal)", false);

			if (opened)
			{
				auto& bic = entity.GetComponent<BlueprintInstanceComponent>();

				ImGui::TextDisabled("Source Path:");
				ImGui::SameLine();
				ImGui::Text("%s", bic.BlueprintPath.c_str());

				ImGui::TextDisabled("Asset UUID:");
				ImGui::SameLine();
				ImGui::Text("%llu", static_cast<uint64_t>(bic.BlueprintAssetUUID));

				ImGui::TextDisabled("Original YAML Size:");
				ImGui::SameLine();
				ImGui::Text("%d bytes", static_cast<int>(bic.OriginalYAML.size()));
			}

			ComponentCard::End();
		}
	};
}
