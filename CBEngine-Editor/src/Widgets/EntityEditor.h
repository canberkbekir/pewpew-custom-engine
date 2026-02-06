#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "AssetPicker.h"

#include <cstring>

namespace CB
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

			// VoxelRenderer component
			DrawVoxelRendererComponent(entity);

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

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_AllowItemOverlap |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!ImGui::TreeNodeEx("Transform", flags))
        return;

    auto& transform = entity.GetComponent<TransformComponent>();

    // --- Table layout: Left = label, Right = controls (aligned) ---
    if (ImGui::BeginTable("##TransformTable", 2,
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // Position
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Position");
        ImGui::TableSetColumnIndex(1);
        DrawVec3Control("##Position", transform.Position);

        // Rotation (store radians, show degrees)
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Rotation");
        ImGui::TableSetColumnIndex(1);

        Vector3 rotationDegrees = glm::degrees(transform.Rotation);
        if (DrawVec3Control("##Rotation", rotationDegrees))
            transform.Rotation = glm::radians(rotationDegrees);

        // Scale with lock button on the right
        static bool scaleLocked = true;

        // Keep lastScale stable across frames AND across entity selection
        static uint64_t s_LastEntityId = 0;
        static Vector3  s_LastScale(1.0f, 1.0f, 1.0f);

        const uint64_t entityId = (uint64_t)entity.GetUUID(); // adjust if your API differs
        if (s_LastEntityId != entityId)
        {
            s_LastEntityId = entityId;
            s_LastScale = transform.Scale;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Scale");
        ImGui::TableSetColumnIndex(1);

        Vector3 currentScale = transform.Scale;

        // Reserve space for lock button at the far right
        const float buttonSize = ImGui::GetFrameHeight();
        const float spacing    = ImGui::GetStyle().ItemInnerSpacing.x;
        const float avail      = ImGui::GetContentRegionAvail().x;
        const float vecWidth   = ImMax(1.0f, avail - (buttonSize + spacing));

        ImGui::BeginGroup();
        ImGui::PushItemWidth(vecWidth);

        const bool scaleEdited = DrawVec3Control("##Scale", currentScale, 1.0f);

        ImGui::PopItemWidth();

        ImGui::SameLine(0.0f, spacing);

        // Icon-only button (no text)
        // TODO: draw your lock/unlock icon here (ImageButton or custom draw)
        if (ImGui::Button("##ScaleLock", ImVec2(buttonSize, buttonSize)))
            scaleLocked = !scaleLocked;

        ImGui::EndGroup();

        if (scaleEdited)
        {
            if (scaleLocked)
            {
                Vector3 delta = currentScale - s_LastScale;

                float ratio = 0.0f;
                if (fabs(delta.x) > 0.0001f && fabs(s_LastScale.x) > 0.000001f)
                    ratio = currentScale.x / s_LastScale.x;
                else if (fabs(delta.y) > 0.0001f && fabs(s_LastScale.y) > 0.000001f)
                    ratio = currentScale.y / s_LastScale.y;
                else if (fabs(delta.z) > 0.0001f && fabs(s_LastScale.z) > 0.000001f)
                    ratio = currentScale.z / s_LastScale.z;

                if (ratio != 0.0f)
                    transform.Scale = s_LastScale * ratio;
            }
            else
            {
                transform.Scale = currentScale;
            }

            s_LastScale = transform.Scale;
        }
        else
        {
            s_LastScale = transform.Scale;
        }

        ImGui::EndTable();
    }

    ImGui::TreePop();
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

				// Mesh picker (ProcessedMesh only)
				if (AssetPicker::DrawMesh("Mesh", meshRenderer.MeshUUID))
				{
					meshRenderer.MeshAsset = nullptr;
					if (meshRenderer.MeshUUID.IsValid())
					{
						const AssetMetadata* meta = AssetManager::GetRegistry().GetMetadata(meshRenderer.MeshUUID);
						if (meta && meta->Type == AssetType::ProcessedMesh)
						{
							auto pmAsset = AssetManager::GetAsset<ProcessedMeshAsset>(meshRenderer.MeshUUID);
							if (pmAsset && !pmAsset->SourceFilePath.empty())
							{
								std::filesystem::path sourcePath = AssetManager::GetAssetDirectory() / pmAsset->SourceFilePath;
								meshRenderer.MeshAsset = Mesh::Load(sourcePath.string());
							}
						}
					}
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

		static void DrawVoxelRendererComponent(Entity entity)
		{
			if (!entity.HasComponent<VoxelRendererComponent>())
				return;

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
				| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

			bool opened = ImGui::TreeNodeEx("Voxel Renderer", flags);

			// Remove button
			ImGui::SameLine(ImGui::GetWindowWidth() - 25);
			if (ImGui::SmallButton("X##RemoveVoxelRenderer"))
			{
				entity.RemoveComponent<VoxelRendererComponent>();
				if (opened) ImGui::TreePop();
				return;
			}

			if (opened)
			{
				auto& voxelRenderer = entity.GetComponent<VoxelRendererComponent>();

				// Visible checkbox
				ImGui::Checkbox("Visible", &voxelRenderer.Visible);

				ImGui::Spacing();

				// VoxelMesh picker (.vmesh only)
				if (AssetPicker::DrawVoxelMesh("Voxel Mesh", voxelRenderer.VoxelMeshUUID))
				{
					voxelRenderer.MeshAsset = nullptr;
					if (voxelRenderer.VoxelMeshUUID.IsValid())
					{
						auto vmAsset = AssetManager::GetAsset<VoxelMeshAsset>(voxelRenderer.VoxelMeshUUID);
						if (vmAsset && vmAsset->VoxelCount > 0)
						{
							voxelRenderer.MeshAsset = VoxelizerAPI::CreateMeshFromGrid(vmAsset->GridData);
							voxelRenderer.VoxelSettings = vmAsset->VoxelSettings;
						}
					}
				}
				if (voxelRenderer.MeshAsset)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("(%u tris)", voxelRenderer.MeshAsset->GetIndexCount() / 3);
				}

				// Material picker
				if (AssetPicker::DrawMaterial("Material", voxelRenderer.MaterialUUID))
				{
					if (voxelRenderer.MaterialUUID.IsValid())
						voxelRenderer.MaterialAsset = AssetManager::GetAsset<Material>(voxelRenderer.MaterialUUID);
					else
						voxelRenderer.MaterialAsset = nullptr;
				}
				if (voxelRenderer.MaterialAsset)
				{
					ImGui::SameLine();
					Vector3 albedo = voxelRenderer.MaterialAsset->GetAlbedo();
					ImGui::ColorButton("##MatPreview", ImVec4(albedo.x, albedo.y, albedo.z, 1.0f),
						ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(16, 16));
				}

				// Shader picker
				if (AssetPicker::DrawShader("Shader", voxelRenderer.ShaderUUID))
				{
					if (voxelRenderer.ShaderUUID.IsValid())
						voxelRenderer.ShaderAsset = AssetManager::GetAsset<Shader>(voxelRenderer.ShaderUUID);
					else
						voxelRenderer.ShaderAsset = nullptr;
				}

				// Show voxel info
				if (voxelRenderer.VoxelMeshUUID.IsValid())
				{
					ImGui::Spacing();
					ImGui::TextDisabled("Grid: %d | %s",
						voxelRenderer.VoxelSettings.GridSize,
						voxelRenderer.VoxelSettings.Solid ? "Solid" : "Surface");
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

				if (!entity.HasComponent<VoxelRendererComponent>())
				{
					if (ImGui::MenuItem("Voxel Renderer"))
					{
						entity.AddComponent<VoxelRendererComponent>();
						ImGui::CloseCurrentPopup();
					}
				}

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
