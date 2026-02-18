#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"

namespace CB
{
    class ColliderComponentView
    {
    public:
        static void Draw(Entity entity)
        {
            if (!entity.HasComponent<ColliderComponent>())
                return;

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_SpanAvailWidth;

            bool opened = ImGui::TreeNodeEx("Collider", flags);

            // Right-aligned remove button
            {
                float lineHeight = ImGui::GetFrameHeight();
                float rightX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - lineHeight;
                ImGui::SameLine(rightX);
                if (ImGui::Button("X##Collider", ImVec2(lineHeight, lineHeight)))
                {
                    entity.RemoveComponent<ColliderComponent>();
                    if (opened) ImGui::TreePop();
                    return;
                }
            }

            if (opened)
            {
                auto& collider = entity.GetComponent<ColliderComponent>();

                // Shape combo
                const char* shapeNames[] = { "Box", "Sphere", "Capsule", "Voxel Compound" };
                int currentShape = static_cast<int>(collider.Shape);
                if (ImGui::Combo("Shape", &currentShape, shapeNames, IM_ARRAYSIZE(shapeNames)))
                {
                    collider.Shape = static_cast<ColliderShape>(currentShape);
                    collider.ShapeDirty = true;
                }

                ImGui::Spacing();

                // Shape-specific parameters
                switch (collider.Shape)
                {
                case ColliderShape::Box:
                    if (ImGui::DragFloat3("Half Extents", &collider.HalfExtents.x, 0.1f, 0.001f, 10000.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    if (ImGui::Button("Auto Fit##Box"))
                    {
                        AutoFitCollider(entity, collider);
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set half extents from mesh bounding box");
                    break;

                case ColliderShape::Sphere:
                    if (ImGui::DragFloat("Radius", &collider.Radius, 0.1f, 0.001f, 10000.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    if (ImGui::Button("Auto Fit##Sphere"))
                    {
                        AutoFitCollider(entity, collider);
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set radius from mesh bounding box");
                    break;

                case ColliderShape::Capsule:
                    if (ImGui::DragFloat("Capsule Radius", &collider.CapsuleRadius, 0.1f, 0.001f, 10000.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    if (ImGui::DragFloat("Half Height", &collider.CapsuleHalfHeight, 0.1f, 0.001f, 10000.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    if (ImGui::Button("Auto Fit##Capsule"))
                    {
                        AutoFitCollider(entity, collider);
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set capsule dimensions from mesh bounding box");
                    break;

                case ColliderShape::VoxelCompound:
                    ImGui::TextDisabled("Auto-generated from voxel grid");
                    break;
                }

                ImGui::Spacing();

                if (ImGui::DragFloat3("Offset", &collider.Offset.x, 0.1f, -10000.0f, 10000.0f, "%.3f"))
                    collider.ShapeDirty = true;

                ImGui::Checkbox("Is Trigger", &collider.IsTrigger);

                ImGui::TreePop();
            }
        }

        static bool GetMeshBounds(Entity entity, Vector3& outMin, Vector3& outMax)
        {
            // Try MeshRendererComponent first
            if (entity.HasComponent<MeshRendererComponent>())
            {
                auto& mrc = entity.GetComponent<MeshRendererComponent>();
                if (mrc.MeshAsset)
                {
                    return mrc.MeshAsset->GetBounds(outMin, outMax);
                }
            }

            // Try VoxelRendererComponent
            if (entity.HasComponent<VoxelRendererComponent>())
            {
                auto& vrc = entity.GetComponent<VoxelRendererComponent>();
                if (vrc.MeshAsset)
                {
                    return vrc.MeshAsset->GetBounds(outMin, outMax);
                }
                // Try computing from voxel grid data
                if (vrc.VoxelMeshUUID.IsValid())
                {
                    auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vrc.VoxelMeshUUID);
                    if (vmesh && vmesh->VoxelCount > 0)
                    {
                        auto& grid = vmesh->GridData;
                        outMin = Vector3(grid.origin);
                        outMax = Vector3(grid.origin) + Vector3(grid.size) * Vector3(grid.voxelSize);
                        return true;
                    }
                }
            }

            return false;
        }

        static void AutoFitCollider(Entity entity, ColliderComponent& collider)
        {
            Vector3 boundsMin, boundsMax;
            if (!GetMeshBounds(entity, boundsMin, boundsMax))
            {
                CB_CORE_WARN("Auto Fit: No mesh data available for bounding box calculation");
                return;
            }

            Vector3 center = (boundsMin + boundsMax) * 0.5f;
            Vector3 halfSize = (boundsMax - boundsMin) * 0.5f;

            switch (collider.Shape)
            {
            case ColliderShape::Box:
                collider.HalfExtents = halfSize;
                collider.Offset = center;
                break;

            case ColliderShape::Sphere:
                collider.Radius = glm::length(halfSize);
                collider.Offset = center;
                break;

            case ColliderShape::Capsule:
                collider.CapsuleRadius = std::max(halfSize.x, halfSize.z);
                collider.CapsuleHalfHeight = halfSize.y;
                collider.Offset = center;
                break;

            default:
                break;
            }

            collider.ShapeDirty = true;
        }
    };
}
