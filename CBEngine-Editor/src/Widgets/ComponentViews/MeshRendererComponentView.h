#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "../AssetPicker.h"

namespace CB
{
    class MeshRendererComponentView
    {
    public:
        static void Draw(Entity entity)
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
                            if (pmAsset && pmAsset->HasGeometryData())
                            {
                                meshRenderer.MeshAsset = pmAsset->CreateMesh();
                            }
                            else if (pmAsset && !pmAsset->SourceFilePath.empty())
                            {
                                std::filesystem::path sourcePath = AssetManager::GetAssetDirectory() / pmAsset->
                                    SourceFilePath;
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
    };
}
