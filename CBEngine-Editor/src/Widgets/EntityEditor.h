#pragma once

#include "imgui.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Components/BlueprintInstanceComponent.h"
#include "CBEngine/Asset/BlueprintAsset.h"
#include "CBEngine/Selection/Selection.h"

// Component card and views
#include "CBEngine/Physics/PhysicsLayers.h"
#include "ComponentCard.h"
#include "ComponentViews/TagComponentView.h"
#include "ComponentViews/TransformComponentView.h"
#include "ComponentViews/DirectionalLightComponentView.h"
#include "ComponentViews/MeshRendererComponentView.h"
#include "ComponentViews/VoxelRendererComponentView.h"
#include "ComponentViews/RigidBodyComponentView.h"
#include "ComponentViews/ColliderComponentView.h"
#include "ComponentViews/ScriptComponentView.h"
#include "ComponentViews/GameManagerComponentView.h"
#include "ComponentViews/CameraComponentView.h"
#include "ComponentViews/BlueprintInstanceComponentView.h"
#include "ComponentViews/HingeJointComponentView.h"
#include "ComponentViews/HingeChainComponentView.h"
#include "ComponentViews/DestructibleVoxelComponentView.h"
#include "ComponentViews/VoxelAnchorComponentView.h"
#include "ComponentViews/WorldGridComponentView.h"
#include "ComponentViews/AddComponentMenu.h"

#include <fstream>

namespace CB
{
	class EntityEditor
	{
	public:
		static void Draw(UUID entityUUID)
		{
			Ref<Scene> scene = SceneManager::GetActiveScene();
			if (!scene) {
				ImGui::TextDisabled("No active scene");
				return;
			}

			Entity entity = scene->GetEntityByUUID(entityUUID);
			if (!entity) {
				ImGui::TextDisabled("Entity not found");
				return;
			}

			Draw(entity);
		}

		static void Draw(Entity entity)
		{
			if (!entity) {
				ImGui::TextDisabled("Invalid entity");
				return;
			}

			// Blueprint instance section (before components)
			DrawBlueprintInstanceSection(entity);

			TagComponentView::Draw(entity);

			// Layer dropdown
			if (entity.HasComponent<IDComponent>()) {
				auto& id = entity.GetComponent<IDComponent>();
				int currentLayer = id.Layer;

				ImGui::Text("Layer");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##Layer", PhysicsLayers::GetLayerName(id.Layer))) {
					for (int i = 0; i < PhysicsLayers::NUM_LAYERS; i++) {
						bool isSelected = (currentLayer == i);
						if (ImGui::Selectable(PhysicsLayers::GetLayerName(static_cast<uint8_t>(i)), isSelected)) {
							uint8_t newLayer = static_cast<uint8_t>(i);
							if (newLayer != id.Layer) {
								id.Layer = newLayer;

								// If entity has children, offer to change their layers too
								if (entity.HasChildren())
									ImGui::OpenPopup("ChangeChildLayers");
							}
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				// Popup for changing children layers
				if (ImGui::BeginPopupModal("ChangeChildLayers", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::Text("Change layer for children too?");
					ImGui::Separator();

					if (ImGui::Button("Yes", ImVec2(120, 0))) {
						SetLayerRecursive(entity, id.Layer);
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("No", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
					ImGui::EndPopup();
				}
			}

			ImGui::Separator();

			TransformComponentView::Draw(entity);
			DirectionalLightComponentView::Draw(entity);
			MeshRendererComponentView::Draw(entity);
			VoxelRendererComponentView::Draw(entity);
			CameraComponentView::Draw(entity);
			RigidBodyComponentView::Draw(entity);
			ColliderComponentView::Draw(entity);
			ScriptComponentView::Draw(entity);
			GameManagerComponentView::Draw(entity);
			HingeJointComponentView::Draw(entity);
			HingeChainComponentView::Draw(entity);
			DestructibleVoxelComponentView::Draw(entity);
			VoxelAnchorComponentView::Draw(entity);
			WorldGridComponentView::Draw(entity);
			BlueprintInstanceComponentView::Draw(entity);

			ImGui::Separator();

			AddComponentMenu::Draw(entity);
		}
	private:
		static void SetLayerRecursive(Entity entity,uint8_t layer)
		{
			for (Entity child : entity.GetChildren()) {
				if (child.HasComponent<IDComponent>())
					child.GetComponent<IDComponent>().Layer = layer;
				if (child.HasChildren())
					SetLayerRecursive(child, layer);
			}
		}

		static void DrawBlueprintInstanceSection(Entity entity)
		{
			if (!entity.HasComponent<BlueprintInstanceComponent>())
				return;

			auto& bic = entity.GetComponent<BlueprintInstanceComponent>();

			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.25f, 0.45f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.3f, 0.55f, 1.0f));

			if (ImGui::CollapsingHeader("Blueprint Instance", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("Source: %s", bic.BlueprintPath.c_str());

				if (bic.BlueprintAssetUUID.IsValid())
					ImGui::TextDisabled("UUID: %llu", static_cast<uint64_t>(bic.BlueprintAssetUUID));

				// Change detection
				String currentYAML = SceneSerializer::SerializeEntityHierarchy(entity);
				bool hasChanges = (currentYAML != bic.OriginalYAML);

				if (hasChanges)
					ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Status: Modified");
				else
					ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Status: Up to date");

				// Apply Overrides button
				if (hasChanges && !bic.BlueprintPath.empty()) {
					if (ImGui::Button("Apply Overrides")) {
						// Re-serialize and save to the .blueprint file
						auto bp = CreateRef<BlueprintAsset>();
						bp->YAMLData = currentYAML;
						bp->RootEntityName = entity.GetName();

						std::filesystem::path savePath(bic.BlueprintPath);
						if (bp->Save(savePath)) {
							bic.OriginalYAML = currentYAML;
							CB_CORE_INFO("Applied blueprint overrides to: {0}", bic.BlueprintPath);
						}
					}

					ImGui::SameLine();
				}

				// Revert to Blueprint button
				if (hasChanges) {
					if (ImGui::Button("Revert to Blueprint")) {
						Ref<Scene> scene = SceneManager::GetActiveScene();
						if (scene) {
							// Save blueprint tracking info before destroying
							UUID bpAssetUUID = bic.BlueprintAssetUUID;
							String bpPath = bic.BlueprintPath;
							String originalYAML = bic.OriginalYAML;

							// Save root entity's current world position
							Vector3 rootPosition = entity.GetComponent<TransformComponent>().Position;
							UUID parentUUID = entity.GetComponent<TransformComponent>().Parent;

							// Destroy the current hierarchy
							scene->DestroyEntity(entity);

							// Re-instantiate from original YAML
							SceneSerializer serializer(scene);
							auto newEntities = serializer.InstantiateBlueprint(originalYAML, bpPath, bpAssetUUID);

							if (!newEntities.empty()) {
								// Restore position and parent
								newEntities[0].GetComponent<TransformComponent>().Position = rootPosition;
								if (parentUUID.IsValid()) {
									Entity parent = scene->GetEntityByUUID(parentUUID);
									if (parent)
										newEntities[0].SetParent(parent);
								}

								Selection::Select(Selectable::Entity(newEntities[0].GetUUID()));
							}
						}
					}
				}
			}

			ImGui::PopStyleColor(2);
			ImGui::Separator();
		}
	};
}