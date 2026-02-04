#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "PewPew/Core/Core.h"
#include "PewPew/Core/UUID.h"
#include "PewPew/Scene/Scene.h"
#include "PewPew/Scene/Entity.h"
#include "PewPew/Scene/SceneManager.h"
#include "PewPew/Components/CoreComponents.h"
#include "PewPew/Components/TransformComponent.h"
#include "PewPew/Components/MeshRendererComponent.h"
#include "PewPew/Asset/AssetManager.h"
#include "AssetPicker.h"

#include <cstring>

namespace PewPew
{
	class EntityEditor
	{
	public:
		// Draw entity editor for an entity UUID
		static void Draw(UUID entityUUID)
		{
			Ref<Scene> scene = SceneManager::GetActiveScene();
			if (!scene)
			{
				ImGui::TextDisabled("No active scene");
				return;
			}

			Entity entity = scene->GetEntityByUUID(entityUUID);
			if (!entity)
			{
				ImGui::TextDisabled("Entity not found");
				return;
			}

			Draw(entity);
		}

		// Draw entity editor for an entity reference
		static void Draw(Entity entity)
		{
			if (!entity)
			{
				ImGui::TextDisabled("Invalid entity");
				return;
			}

			// Tag/Name component
			DrawTagComponent(entity);

			ImGui::Separator();

			// Transform component
			DrawTransformComponent(entity);

			// MeshRenderer component
			DrawMeshRendererComponent(entity);

			// Add more component editors here as you implement them
			// DrawRigidbodyComponent(entity);
			// DrawColliderComponent(entity);
			// DrawLightComponent(entity);

			ImGui::Separator();

			// Add Component button
			DrawAddComponentButton(entity);
		}

	private:
		static void DrawTagComponent(Entity entity)
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

		static void DrawTransformComponent(Entity entity)
		{
			if (!entity.HasComponent<TransformComponent>())
				return;

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
				| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

			bool opened = ImGui::TreeNodeEx("Transform", flags);

			if (opened)
			{
				auto& transform = entity.GetComponent<TransformComponent>();

				// Position
				DrawVec3Control("Position", transform.Position);

				// Rotation (convert to degrees for display)
				Vector3 rotationDegrees = glm::degrees(transform.Rotation);
				if (DrawVec3Control("Rotation", rotationDegrees))
				{
					transform.Rotation = glm::radians(rotationDegrees);
				}

				// Scale
				DrawVec3Control("Scale", transform.Scale, 1.0f);

				ImGui::TreePop();
			}
		}

		static void DrawMeshRendererComponent(Entity entity)
		{
			if (!entity.HasComponent<MeshRendererComponent>())
				return;

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
				| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

			bool opened = ImGui::TreeNodeEx("Mesh Renderer", flags);

			// Remove button
			ImGui::SameLine(ImGui::GetWindowWidth() - 25);
			if (ImGui::SmallButton("X##RemoveMeshRenderer"))
			{
				entity.RemoveComponent<MeshRendererComponent>();
				if (opened) ImGui::TreePop();
				return;
			}

			if (opened)
			{
				auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();

				// Visible checkbox
				ImGui::Checkbox("Visible", &meshRenderer.Visible);

				ImGui::Spacing();

				// Mesh picker
				if (AssetPicker::DrawMesh("Mesh", meshRenderer.MeshUUID))
				{
					if (meshRenderer.MeshUUID.IsValid())
						meshRenderer.MeshAsset = AssetManager::GetAsset<Mesh>(meshRenderer.MeshUUID);
					else
						meshRenderer.MeshAsset = nullptr;
				}
				if (meshRenderer.MeshAsset)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("(%u tris)", meshRenderer.MeshAsset->GetIndexCount() / 3);
				}

				// Material picker
				if (AssetPicker::DrawMaterial("Material", meshRenderer.MaterialUUID))
				{
					if (meshRenderer.MaterialUUID.IsValid())
						meshRenderer.MaterialAsset = AssetManager::GetAsset<Material>(meshRenderer.MaterialUUID);
					else
						meshRenderer.MaterialAsset = nullptr;
				}
				if (meshRenderer.MaterialAsset)
				{
					ImGui::SameLine();
					Vector3 albedo = meshRenderer.MaterialAsset->GetAlbedo();
					ImGui::ColorButton("##MatPreview", ImVec4(albedo.x, albedo.y, albedo.z, 1.0f),
						ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(16, 16));
				}

				// Shader picker
				if (AssetPicker::DrawShader("Shader", meshRenderer.ShaderUUID))
				{
					if (meshRenderer.ShaderUUID.IsValid())
						meshRenderer.ShaderAsset = AssetManager::GetAsset<Shader>(meshRenderer.ShaderUUID);
					else
						meshRenderer.ShaderAsset = nullptr;
				}

				ImGui::TreePop();
			}
		}

		static void DrawAddComponentButton(Entity entity)
		{
			float buttonWidth = ImGui::GetContentRegionAvail().x;

			if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0)))
			{
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				if (!entity.HasComponent<TransformComponent>())
				{
					if (ImGui::MenuItem("Transform"))
					{
						entity.AddComponent<TransformComponent>();
						ImGui::CloseCurrentPopup();
					}
				}

				if (!entity.HasComponent<MeshRendererComponent>())
				{
					if (ImGui::MenuItem("Mesh Renderer"))
					{
						entity.AddComponent<MeshRendererComponent>();
						ImGui::CloseCurrentPopup();
					}
				}

				// Add more components here as you implement them
				ImGui::Separator();
				ImGui::TextDisabled("More components coming soon...");

				ImGui::EndPopup();
			}
		}

		// Helper function to draw a Vec3 control with colored labels
		static bool DrawVec3Control(const char* label, Vector3& values, float resetValue = 0.0f)
		{
			bool changed = false;

			ImGui::PushID(label);

			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 80.0f);
			ImGui::Text("%s", label);
			ImGui::NextColumn();

			ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

			// X
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
			if (ImGui::Button("X", buttonSize))
			{
				values.x = resetValue;
				changed = true;
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			if (ImGui::DragFloat("##X", &values.x, 0.1f)) changed = true;
			ImGui::PopItemWidth();
			ImGui::SameLine();

			// Y
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
			if (ImGui::Button("Y", buttonSize))
			{
				values.y = resetValue;
				changed = true;
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			if (ImGui::DragFloat("##Y", &values.y, 0.1f)) changed = true;
			ImGui::PopItemWidth();
			ImGui::SameLine();

			// Z
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
			if (ImGui::Button("Z", buttonSize))
			{
				values.z = resetValue;
				changed = true;
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			if (ImGui::DragFloat("##Z", &values.z, 0.1f)) changed = true;
			ImGui::PopItemWidth();

			ImGui::PopStyleVar();
			ImGui::Columns(1);
			ImGui::PopID();

			return changed;
		}
	};
}
