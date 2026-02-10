#pragma once

#include "imgui.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Asset/AssetMetadata.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "AssetPicker.h"
#include "../EditorLayer.h"

#include <algorithm>
#include <filesystem>

namespace CB
{
    class MeshEditor
    {
    public:
        static void Draw(UUID assetUUID)
        {
            const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(assetUUID);
            if (!metadata)
            {
                ImGui::TextDisabled("Invalid asset");
                return;
            }

            switch (metadata->Type)
            {
            case AssetType::Mesh:
                DrawRawMesh(assetUUID, *metadata);
                break;
            case AssetType::ProcessedMesh:
                DrawProcessedMesh(assetUUID, *metadata);
                break;
            case AssetType::VoxelMesh:
                DrawVoxelMesh(assetUUID, *metadata);
                break;
            default:
                ImGui::TextDisabled("Not a mesh asset");
                break;
            }
        }

    private:
        // ============================================================
        // Raw Mesh (.fbx, .obj, .gltf, .glb)
        // ============================================================
        static void DrawRawMesh(UUID meshUUID, const AssetMetadata& metadata)
        {
            Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshUUID);
            if (!mesh)
            {
                ImGui::TextDisabled("Failed to load mesh");
                return;
            }

            // Header
            ImGui::Text("Mesh: %s", metadata.FilePath.stem().string().c_str());
            ImGui::TextDisabled("Path: %s", metadata.FilePath.string().c_str());
            ImGui::Separator();

            // Statistics
            if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Vertex Count: %u", mesh->GetVertexCount());
                ImGui::Text("Index Count: %u", mesh->GetIndexCount());
                ImGui::Text("Triangle Count: %u", mesh->GetIndexCount() / 3);

                if (mesh->HasCPUData())
                    ImGui::TextDisabled("CPU data retained");
                else
                    ImGui::TextDisabled("CPU data released (GPU only)");
            }

            // Process button — opens MeshImportPanel
            ImGui::Separator();
            if (ImGui::Button("Process...", ImVec2(-1, 0)))
            {
                // Queue the import preview via EditorLayer
                // We need the full path from the asset directory
                std::filesystem::path fullPath = AssetManager::GetAssetDirectory() / metadata.FilePath;

                // Use the static function from EditorLayer
                // (included transitively through the editor build)
                EditorLayer::RequestImportPreview(fullPath);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Open Import Preview to create .mesh or .vmesh");
        }

        // ============================================================
        // Processed Mesh (.mesh)
        // ============================================================
        static void DrawProcessedMesh(UUID assetUUID, const AssetMetadata& metadata)
        {
            Ref<ProcessedMeshAsset> asset = AssetManager::GetAsset<ProcessedMeshAsset>(assetUUID);
            if (!asset)
            {
                ImGui::TextDisabled("Failed to load processed mesh");
                return;
            }

            bool modified = false;

            // Header
            ImGui::Text("Processed Mesh: %s", metadata.FilePath.stem().string().c_str());
            ImGui::TextDisabled("Path: %s", metadata.FilePath.string().c_str());
            ImGui::Separator();

            // Source info
            if (ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TextDisabled("Source: %s", asset->SourceFilePath.c_str());
                if (asset->SourceMeshUUID.IsValid())
                    ImGui::TextDisabled("UUID: %llu", static_cast<uint64_t>(asset->SourceMeshUUID));

                // Import settings (read-only)
                ImGui::TextDisabled("Scale: %.2f", asset->ImportSettings.Scale);
                ImGui::TextDisabled("Normals: %s", asset->ImportSettings.GenerateNormals ? "Yes" : "No");
                ImGui::TextDisabled("Tangents: %s", asset->ImportSettings.CalcTangents ? "Yes" : "No");
            }

            // Material Slots
            if (ImGui::CollapsingHeader("Material Slots", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (size_t i = 0; i < asset->MaterialSlots.size(); i++)
                {
                    ImGui::PushID(static_cast<int>(i));
                    String label = asset->MaterialSlots[i].Name.empty()
                                       ? "Slot " + std::to_string(i)
                                       : asset->MaterialSlots[i].Name;

                    ImGui::Text("%s:", label.c_str());
                    ImGui::SameLine();
                    if (AssetPicker::DrawMaterial("##MatSlot", asset->MaterialSlots[i].MaterialUUID))
                        modified = true;

                    ImGui::PopID();
                }
            }

            // Texture Slots
            if (ImGui::CollapsingHeader("Texture Slots", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (size_t i = 0; i < asset->TextureSlots.size(); i++)
                {
                    ImGui::PushID(static_cast<int>(i) + 1000);
                    String label = asset->TextureSlots[i].Name.empty()
                                       ? "Slot " + std::to_string(i)
                                       : asset->TextureSlots[i].Name;

                    ImGui::Text("%s:", label.c_str());
                    ImGui::SameLine();
                    if (AssetPicker::DrawTexture("##TexSlot", asset->TextureSlots[i].TextureUUID))
                        modified = true;

                    ImGui::PopID();
                }
            }

            ImGui::Separator();

            // Re-import button
            if (ImGui::Button("Re-import"))
            {
                EditorLayer::RequestImportPreviewReimport({}, assetUUID);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Open Import Preview with current settings");

            ImGui::SameLine();

            // Save button
            if (modified)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
            }
            if (ImGui::Button("Save"))
            {
                std::filesystem::path fullPath = AssetManager::GetAssetDirectory() / metadata.FilePath;
                asset->Save(fullPath);

                // Update dependencies
                std::vector<UUID> deps;
                if (asset->SourceMeshUUID.IsValid()) deps.push_back(asset->SourceMeshUUID);
                for (const auto& slot : asset->MaterialSlots)
                    if (slot.MaterialUUID.IsValid()) deps.push_back(slot.MaterialUUID);
                for (const auto& slot : asset->TextureSlots)
                    if (slot.TextureUUID.IsValid()) deps.push_back(slot.TextureUUID);

                AssetManager::GetRegistry().UpdateDependencies(assetUUID, deps);
            }
            if (modified)
            {
                ImGui::PopStyleColor(2);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Unsaved changes");
            }
        }

        // ============================================================
        // Voxel Mesh (.vmesh)
        // ============================================================
        static void DrawVoxelMesh(UUID assetUUID, const AssetMetadata& metadata)
        {
            Ref<VoxelMeshAsset> asset = AssetManager::GetAsset<VoxelMeshAsset>(assetUUID);
            if (!asset)
            {
                ImGui::TextDisabled("Failed to load voxel mesh");
                return;
            }

            bool modified = false;

            // Header
            ImGui::Text("Voxel Mesh: %s", metadata.FilePath.stem().string().c_str());
            ImGui::TextDisabled("Path: %s", metadata.FilePath.string().c_str());
            ImGui::Separator();

            // Source info
            if (ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TextDisabled("Source: %s", asset->SourceFilePath.c_str());
                if (asset->SourceMeshUUID.IsValid())
                    ImGui::TextDisabled("UUID: %llu", static_cast<uint64_t>(asset->SourceMeshUUID));
            }

            // Voxel settings (read-only display)
            if (ImGui::CollapsingHeader("Voxel Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TextDisabled("Grid Size: %d", asset->VoxelSettings.GridSize);
                ImGui::TextDisabled("Solid: %s", asset->VoxelSettings.Solid ? "Yes" : "No");
                ImGui::TextDisabled("Padding: %.3f", asset->VoxelSettings.Padding);
                ImGui::TextDisabled("Voxel Count: %llu", asset->VoxelCount);

                if (asset->GridData.size.x > 0)
                {
                    ImGui::TextDisabled("Grid Dimensions: %dx%dx%d",
                                        asset->GridData.size.x, asset->GridData.size.y, asset->GridData.size.z);
                }

                if (asset->ColorTextureUUID.IsValid())
                {
                    const AssetMetadata* texMeta = AssetManager::GetRegistry().GetMetadata(asset->ColorTextureUUID);
                    if (texMeta)
                        ImGui::TextDisabled("Color Texture: %s", texMeta->FilePath.stem().string().c_str());
                }
            }

            // Material Slots
            if (ImGui::CollapsingHeader("Material Slots", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (size_t i = 0; i < asset->MaterialSlots.size(); i++)
                {
                    ImGui::PushID(static_cast<int>(i));
                    String label = asset->MaterialSlots[i].Name.empty()
                                       ? "Slot " + std::to_string(i)
                                       : asset->MaterialSlots[i].Name;

                    ImGui::Text("%s:", label.c_str());
                    ImGui::SameLine();
                    if (AssetPicker::DrawMaterial("##MatSlot", asset->MaterialSlots[i].MaterialUUID))
                        modified = true;

                    ImGui::PopID();
                }
            }

            // Texture Slots
            if (ImGui::CollapsingHeader("Texture Slots", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (size_t i = 0; i < asset->TextureSlots.size(); i++)
                {
                    ImGui::PushID(static_cast<int>(i) + 1000);
                    String label = asset->TextureSlots[i].Name.empty()
                                       ? "Slot " + std::to_string(i)
                                       : asset->TextureSlots[i].Name;

                    ImGui::Text("%s:", label.c_str());
                    ImGui::SameLine();
                    if (AssetPicker::DrawTexture("##TexSlot", asset->TextureSlots[i].TextureUUID))
                        modified = true;

                    ImGui::PopID();
                }
            }

            ImGui::Separator();

            // Re-import button
            if (ImGui::Button("Re-import"))
            {
                EditorLayer::RequestImportPreviewReimport({}, assetUUID);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Open Import Preview with current settings");

            ImGui::SameLine();

            // Save button
            if (modified)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
            }
            if (ImGui::Button("Save"))
            {
                std::filesystem::path fullPath = AssetManager::GetAssetDirectory() / metadata.FilePath;
                asset->Save(fullPath);

                // Update dependencies
                std::vector<UUID> deps;
                if (asset->SourceMeshUUID.IsValid()) deps.push_back(asset->SourceMeshUUID);
                if (asset->ColorTextureUUID.IsValid()) deps.push_back(asset->ColorTextureUUID);
                for (const auto& slot : asset->MaterialSlots)
                    if (slot.MaterialUUID.IsValid()) deps.push_back(slot.MaterialUUID);
                for (const auto& slot : asset->TextureSlots)
                    if (slot.TextureUUID.IsValid()) deps.push_back(slot.TextureUUID);

                AssetManager::GetRegistry().UpdateDependencies(assetUUID, deps);
            }
            if (modified)
            {
                ImGui::PopStyleColor(2);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Unsaved changes");
            }
        }
    };
}
