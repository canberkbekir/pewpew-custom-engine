#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ColliderComponent.h"

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
                    if (ImGui::DragFloat3("Half Extents", &collider.HalfExtents.x, 0.01f, 0.001f, 100.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    break;

                case ColliderShape::Sphere:
                    if (ImGui::DragFloat("Radius", &collider.Radius, 0.01f, 0.001f, 100.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    break;

                case ColliderShape::Capsule:
                    if (ImGui::DragFloat("Capsule Radius", &collider.CapsuleRadius, 0.01f, 0.001f, 100.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    if (ImGui::DragFloat("Half Height", &collider.CapsuleHalfHeight, 0.01f, 0.001f, 100.0f, "%.3f"))
                        collider.ShapeDirty = true;
                    break;

                case ColliderShape::VoxelCompound:
                    ImGui::TextDisabled("Auto-generated from voxel grid");
                    break;
                }

                ImGui::Spacing();

                if (ImGui::DragFloat3("Offset", &collider.Offset.x, 0.01f, -100.0f, 100.0f, "%.3f"))
                    collider.ShapeDirty = true;

                ImGui::Checkbox("Is Trigger", &collider.IsTrigger);

                ImGui::TreePop();
            }
        }
    };
}
