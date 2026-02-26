#pragma once

#include "imgui.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/BlueprintAsset.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Selection/Selection.h"

#include <yaml-cpp/yaml.h>

namespace CB
{
	class BlueprintViewer
	{
	public:
		static void Draw(UUID assetUUID)
		{
			auto bp = AssetManager::GetAsset<BlueprintAsset>(assetUUID);
			if (!bp) {
				// Try loading metadata for display
				const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(assetUUID);
				if (metadata) {
					ImGui::Text("Blueprint");
					ImGui::Separator();
					ImGui::Text("UUID: %llu", static_cast<uint64_t>(assetUUID));
					ImGui::Text("Path: %s", metadata->FilePath.string().c_str());
					ImGui::Separator();
					ImGui::TextDisabled("Failed to load blueprint asset");
				}
				return;
			}

			ImGui::Text("Blueprint: %s", bp->RootEntityName.c_str());
			ImGui::Separator();

			const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(assetUUID);
			if (metadata)
				ImGui::Text("Path: %s", metadata->FilePath.string().c_str());

			ImGui::Text("UUID: %llu", static_cast<uint64_t>(assetUUID));
			ImGui::Text("Entities: %d", bp->EntityCount);

			ImGui::Separator();

			// Entity hierarchy tree (read-only preview)
			if (!bp->YAMLData.empty()) {
				if (ImGui::TreeNodeEx("Entity Hierarchy", ImGuiTreeNodeFlags_DefaultOpen)) {
					try {
						YAML::Node data = YAML::Load(bp->YAMLData);
						auto entities = data["Entities"];
						if (entities && entities.IsSequence()) {
							for (auto entityNode : entities) {
								String name = "Entity";
								if (entityNode["TagComponent"] && entityNode["TagComponent"]["Tag"])
									name = entityNode["TagComponent"]["Tag"].as<std::string>();

								uint64_t uuid = entityNode["Entity"].as<uint64_t>(0);

								// Check if entity has parent (is a child)
								bool hasParent = false;
								if (entityNode["TransformComponent"] && entityNode["TransformComponent"]["Parent"]) {
									uint64_t parentUUID = entityNode["TransformComponent"]["Parent"].as<uint64_t>(0);
									hasParent = parentUUID != 0;
								}

								if (hasParent)
									ImGui::TextDisabled("  | %s", name.c_str());
								else
									ImGui::BulletText("%s", name.c_str());

								// Show components on hover
								if (ImGui::IsItemHovered()) {
									ImGui::BeginTooltip();
									ImGui::Text("UUID: %llu", uuid);
									if (entityNode["MeshRendererComponent"])
										ImGui::Text("+ MeshRendererComponent");
									if (entityNode["VoxelRendererComponent"])
										ImGui::Text("+ VoxelRendererComponent");
									if (entityNode["DirectionalLightComponent"])
										ImGui::Text("+ DirectionalLightComponent");
									if (entityNode["RigidBodyComponent"])
										ImGui::Text("+ RigidBodyComponent");
									if (entityNode["ColliderComponent"])
										ImGui::Text("+ ColliderComponent");
									if (entityNode["ScriptComponent"])
										ImGui::Text("+ ScriptComponent");
									ImGui::EndTooltip();
								}
							}
						}
					}
					catch (const YAML::ParserException&) { ImGui::TextDisabled("Failed to parse blueprint YAML"); }

					ImGui::TreePop();
				}
			}

			ImGui::Separator();

			// Instantiate button
			if (ImGui::Button("Instantiate in Scene")) {
				Ref<Scene> scene = SceneManager::GetActiveScene();
				if (scene) {
					SceneSerializer serializer(scene);
					String bpPath;
					if (metadata)
						bpPath = metadata->FilePath.string();
					auto entities = serializer.InstantiateBlueprint(bp->YAMLData, bpPath, assetUUID);
					if (!entities.empty()) { Selection::Select(Selectable::Entity(entities[0].GetUUID())); }
				}
			}
		}
	};
}