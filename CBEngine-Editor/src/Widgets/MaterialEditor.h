#pragma once

#include "imgui.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Asset/AssetMetadata.h"
#include "CBEngine/Asset/AssetManager.h"
#include "AssetPicker.h"

namespace CB
{
    class MaterialEditor
    {
    public:
        // Draw material editor for an asset UUID
        // Returns true if material was modified
        static bool Draw(UUID materialUUID)
        {
            const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(materialUUID);
            if (!metadata || metadata->Type != AssetType::Material)
            {
                ImGui::TextDisabled("Invalid material");
                return false;
            }

            Ref<Material> material = AssetManager::GetAsset<Material>(materialUUID);
            if (!material)
            {
                ImGui::TextDisabled("Failed to load material");
                return false;
            }

            return Draw(material, *metadata);
        }

        // Draw material editor for a material reference
        static bool Draw(Ref<Material> material, const AssetMetadata& metadata)
        {
            if (!material)
            {
                ImGui::TextDisabled("No material");
                return false;
            }

            bool modified = false;

            // Get material directory for relative path calculation
            std::filesystem::path matDir = metadata.FilePath.parent_path();

            // Helper to compute relative path from material directory to texture
            auto makeRelativePath = [&matDir](const String& texturePath) -> String
            {
                if (texturePath.empty()) return "";
                std::filesystem::path texPath(texturePath);
                // Compute relative path from material directory to texture
                std::filesystem::path relativePath = relative(texPath, matDir);
                // Normalize to forward slashes for cross-platform compatibility
                String result = relativePath.generic_string();
                return result;
            };

            // Header
            ImGui::Text("Material: %s", metadata.FilePath.stem().string().c_str());
            ImGui::TextDisabled("Path: %s", metadata.FilePath.string().c_str());
            ImGui::Separator();

            // Base Color / Albedo
            if (ImGui::CollapsingHeader("Base Color", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Vector3 albedo = material->GetAlbedo();
                if (ImGui::ColorEdit3("Albedo Color", &albedo.x))
                {
                    material->SetAlbedo(albedo);
                    modified = true;
                }

                // Albedo texture with picker
                if (DrawTextureSlot("Albedo Map", material->GetAlbedoMap(),
                                    [&material, &makeRelativePath](Ref<Texture2D> tex, const String& path)
                                    {
                                        material->SetAlbedoMap(tex, makeRelativePath(path));
                                    }))
                {
                    modified = true;
                }
            }

            // PBR Properties
            if (ImGui::CollapsingHeader("PBR Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Metallic
                float metallic = material->GetMetallic();
                if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
                {
                    material->SetMetallic(metallic);
                    modified = true;
                }

                // Metallic map
                if (DrawTextureSlot("Metallic Map", material->GetMetallicMap(),
                                    [&material, &makeRelativePath](Ref<Texture2D> tex, const String& path)
                                    {
                                        material->SetMetallicMap(tex, makeRelativePath(path));
                                    }))
                {
                    modified = true;
                }

                ImGui::Spacing();

                // Roughness
                float roughness = material->GetRoughness();
                if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
                {
                    material->SetRoughness(roughness);
                    modified = true;
                }

                // Roughness map
                if (DrawTextureSlot("Roughness Map", material->GetRoughnessMap(),
                                    [&material, &makeRelativePath](Ref<Texture2D> tex, const String& path)
                                    {
                                        material->SetRoughnessMap(tex, makeRelativePath(path));
                                    }))
                {
                    modified = true;
                }
            }

            // Normal Map
            if (ImGui::CollapsingHeader("Normal Map", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (DrawTextureSlot("Normal Map", material->GetNormalMap(),
                                    [&material, &makeRelativePath](Ref<Texture2D> tex, const String& path)
                                    {
                                        material->SetNormalMap(tex, makeRelativePath(path));
                                    }))
                {
                    modified = true;
                }
            }

            // Additional Settings
            if (ImGui::CollapsingHeader("Additional Settings"))
            {
                float smoothShading = material->GetSmoothShading();
                if (ImGui::SliderFloat("Smooth Shading", &smoothShading, 0.0f, 1.0f))
                {
                    material->SetSmoothShading(smoothShading);
                    modified = true;
                }
            }

            ImGui::Separator();

            // Save button
            if (modified)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
            }

            if (ImGui::Button("Save Material", ImVec2(-1, 0)))
            {
                std::filesystem::path fullPath = AssetManager::GetAssetDirectory() / metadata.FilePath;
                material->Save(fullPath.string());
            }

            if (modified)
            {
                ImGui::PopStyleColor(2);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Unsaved changes");
            }

            return modified;
        }

    private:
        // Helper to draw a texture slot with preview, picker, drag-drop, and clear button
        // Returns true if the texture was changed
        // SetterFunc signature: void(Ref<Texture2D> texture, const String& relativePath)
        template <typename SetterFunc>
        static bool DrawTextureSlot(const char* label, const Ref<Texture2D>& currentTexture, SetterFunc setter)
        {
            bool changed = false;

            ImGui::PushID(label);

            ImGui::Text("%s", label);

            // Texture preview / drop zone
            ImVec2 previewSize(64, 64);

            if (currentTexture)
            {
                // Show texture preview
                ImGui::Image(
                    reinterpret_cast<void*>(static_cast<uint64_t>(currentTexture->GetRendererID())),
                    previewSize, ImVec2(0, 1), ImVec2(1, 0)
                );
            }
            else
            {
                // Empty slot - show drop zone
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::Button("Drop Here", previewSize);
                ImGui::PopStyleColor(2);
            }

            // Drag-drop target - accept both ASSET_UUID and CONTENT_BROWSER_ITEM
            if (ImGui::BeginDragDropTarget())
            {
                // Try ASSET_UUID first
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_UUID"))
                {
                    UUID textureUUID = *static_cast<UUID*>(payload->Data);
                    const AssetMetadata* texMeta = AssetManager::GetRegistry().GetMetadata(textureUUID);
                    if (texMeta && texMeta->Type == AssetType::Texture2D)
                    {
                        auto texture = AssetManager::GetAsset<Texture2D>(textureUUID);
                        setter(texture, texMeta->FilePath.string());
                        changed = true;
                    }
                }
                // Try CONTENT_BROWSER_ITEM (file path from content browser)
                else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    std::filesystem::path filePath = static_cast<const char*>(payload->Data);
                    // Check if it's a texture file
                    String ext = filePath.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
                    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp")
                    {
                        // Get relative path from assets folder
                        std::filesystem::path relativePath = relative(filePath, AssetManager::GetAssetDirectory());

                        // Try to find in asset registry or load directly
                        UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);
                        if (uuid.IsValid())
                        {
                            auto texture = AssetManager::GetAsset<Texture2D>(uuid);
                            setter(texture, relativePath.string());
                            changed = true;
                        }
                        else
                        {
                            // Load texture directly
                            auto texture = Texture2D::Create(filePath.string());
                            if (texture)
                            {
                                setter(texture, relativePath.string());
                                changed = true;
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Buttons row
            ImGui::SameLine();
            ImGui::BeginGroup();

            // Browse button - opens asset picker
            if (ImGui::Button("Browse"))
            {
                ImGui::OpenPopup("TexturePicker");
            }

            // Clear button
            if (currentTexture)
            {
                if (ImGui::Button("Clear"))
                {
                    setter(nullptr, "");
                    changed = true;
                }
            }

            ImGui::EndGroup();

            // Texture picker popup
            ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Appearing);
            if (ImGui::BeginPopup("TexturePicker"))
            {
                static char searchBuffer[256] = "";

                // Search bar with auto-focus
                if (ImGui::IsWindowAppearing())
                {
                    searchBuffer[0] = '\0';
                    ImGui::SetKeyboardFocusHere();
                }

                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##Search", "Search textures...", searchBuffer, sizeof(searchBuffer));

                ImGui::Separator();

                // "None" option
                if (ImGui::Selectable("None (Clear)"))
                {
                    setter(nullptr, "");
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::Separator();

                // List textures
                String searchLower = searchBuffer;
                std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), tolower);

                ImGui::BeginChild("TextureList", ImVec2(0, 0), false);

                const auto& registry = AssetManager::GetRegistry();
                for (const auto& [uuid, meta] : registry.GetAllAssets())
                {
                    if (meta.Type != AssetType::Texture2D)
                        continue;

                    String assetName = meta.FilePath.stem().string();

                    // Apply search filter
                    if (!searchLower.empty())
                    {
                        String nameLower = assetName;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), tolower);
                        if (nameLower.find(searchLower) == String::npos)
                            continue;
                    }

                    // Show texture with small preview
                    auto texture = AssetManager::GetAsset<Texture2D>(uuid);
                    if (texture)
                    {
                        ImGui::Image(
                            reinterpret_cast<void*>(static_cast<uint64_t>(texture->GetRendererID())),
                            ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0)
                        );
                        ImGui::SameLine();
                    }

                    if (ImGui::Selectable(assetName.c_str()))
                    {
                        setter(texture, meta.FilePath.string());
                        changed = true;
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", meta.FilePath.string().c_str());
                        if (texture)
                        {
                            ImGui::Text("Size: %dx%d", texture->GetWidth(), texture->GetHeight());
                            ImGui::Image(
                                reinterpret_cast<void*>(static_cast<uint64_t>(texture->GetRendererID())),
                                ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0)
                            );
                        }
                        ImGui::EndTooltip();
                    }
                }

                ImGui::EndChild();
                ImGui::EndPopup();
            }

            ImGui::PopID();
            return changed;
        }
    };
}
