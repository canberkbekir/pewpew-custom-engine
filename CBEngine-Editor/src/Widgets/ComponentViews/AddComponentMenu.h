#pragma once

#include "imgui.h"
#include "ColliderComponentView.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Components/CameraComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/AssetRegistry.h"

#include <algorithm>
#include <cstring>

namespace CB
{
    class AddComponentMenu
    {
    public:
        static void Draw(Entity entity)
        {
            float buttonWidth = ImGui::GetContentRegionAvail().x;

            if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0)))
            {
                ImGui::OpenPopup("AddComponentPopup");
                s_SearchBuffer[0] = '\0';
            }

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                // Search filter
                ImGui::SetNextItemWidth(-1);
                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere();
                ImGui::InputTextWithHint("##SearchComponents", "Search...", s_SearchBuffer, sizeof(s_SearchBuffer));

                ImGui::Separator();

                bool hasFilter = s_SearchBuffer[0] != '\0';

                // --- Components Section ---
                bool anyComponentShown = false;

                if (!hasFilter || MatchesFilter("Transform"))
                {
                    if (!entity.HasComponent<TransformComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Transform"))
                        {
                            entity.AddComponent<TransformComponent>();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (!hasFilter || MatchesFilter("Mesh Renderer"))
                {
                    if (!entity.HasComponent<MeshRendererComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Mesh Renderer"))
                        {
                            entity.AddComponent<MeshRendererComponent>();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (!hasFilter || MatchesFilter("Voxel Renderer"))
                {
                    if (!entity.HasComponent<VoxelRendererComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Voxel Renderer"))
                        {
                            entity.AddComponent<VoxelRendererComponent>();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (!hasFilter || MatchesFilter("Directional Light"))
                {
                    if (!entity.HasComponent<DirectionalLightComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Directional Light"))
                        {
                            entity.AddComponent<DirectionalLightComponent>();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (!hasFilter || MatchesFilter("Camera"))
                {
                    if (!entity.HasComponent<CameraComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Camera"))
                        {
                            entity.AddComponent<CameraComponent>();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (!hasFilter || MatchesFilter("Rigid Body"))
                {
                    if (!entity.HasComponent<RigidBodyComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Rigid Body"))
                        {
                            entity.AddComponent<RigidBodyComponent>();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (!hasFilter || MatchesFilter("Collider"))
                {
                    if (!entity.HasComponent<ColliderComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Collider"))
                        {
                            auto& collider = entity.AddComponent<ColliderComponent>();
                            ColliderComponentView::AutoFitCollider(entity, collider);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (!hasFilter || MatchesFilter("Script"))
                {
                    if (!entity.HasComponent<ScriptComponent>())
                    {
                        anyComponentShown = true;
                        if (ImGui::MenuItem("Script"))
                        {
                            entity.AddComponent<ScriptComponent>();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                // --- Scripts Section ---
                // Discover .lua scripts from asset registry
                const auto& allAssets = AssetManager::GetRegistry().GetAllAssets();
                bool hasScripts = false;

                for (const auto& [uuid, metadata] : allAssets)
                {
                    if (metadata.Type == AssetType::Script)
                    {
                        hasScripts = true;
                        break;
                    }
                }

                if (hasScripts)
                {
                    ImGui::Separator();
                    ImGui::TextDisabled("Scripts");

                    for (const auto& [uuid, metadata] : allAssets)
                    {
                        if (metadata.Type != AssetType::Script)
                            continue;

                        std::string filename = metadata.FilePath.filename().string();
                        std::string fullPath = metadata.FilePath.string();

                        if (hasFilter && !MatchesFilter(filename.c_str()))
                            continue;

                        if (ImGui::MenuItem(filename.c_str()))
                        {
                            if (!entity.HasComponent<ScriptComponent>())
                                entity.AddComponent<ScriptComponent>();

                            auto& script = entity.GetComponent<ScriptComponent>();
                            script.ScriptPath = fullPath;
                            script.ScriptLoaded = false;
                            ImGui::CloseCurrentPopup();
                        }

                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("%s", fullPath.c_str());
                    }
                }

                ImGui::EndPopup();
            }
        }

    private:
        static inline char s_SearchBuffer[128] = {};

        static bool MatchesFilter(const char* text)
        {
            if (s_SearchBuffer[0] == '\0')
                return true;

            // Case-insensitive substring match
            std::string lower_text(text);
            std::string lower_filter(s_SearchBuffer);
            std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
            std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(), ::tolower);
            return lower_text.find(lower_filter) != std::string::npos;
        }
    };
}
