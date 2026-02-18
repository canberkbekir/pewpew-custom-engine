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

                if (!entity.HasComponent<DirectionalLightComponent>())
                {
                    if (ImGui::MenuItem("Directional Light"))
                    {
                        entity.AddComponent<DirectionalLightComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::Separator();

                if (!entity.HasComponent<RigidBodyComponent>())
                {
                    if (ImGui::MenuItem("Rigid Body"))
                    {
                        entity.AddComponent<RigidBodyComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!entity.HasComponent<ColliderComponent>())
                {
                    if (ImGui::MenuItem("Collider"))
                    {
                        auto& collider = entity.AddComponent<ColliderComponent>();
                        ColliderComponentView::AutoFitCollider(entity, collider);
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::Separator();

                if (!entity.HasComponent<ScriptComponent>())
                {
                    if (ImGui::MenuItem("Script"))
                    {
                        entity.AddComponent<ScriptComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }
        }
    };
}
