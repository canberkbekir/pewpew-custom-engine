#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/RigidBodyComponent.h"

namespace CB
{
    class RigidBodyComponentView
    {
    public:
        static void Draw(Entity entity)
        {
            if (!entity.HasComponent<RigidBodyComponent>())
                return;

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_SpanAvailWidth;

            bool opened = ImGui::TreeNodeEx("Rigid Body", flags);

            // Right-aligned remove button
            {
                float lineHeight = ImGui::GetFrameHeight();
                float rightX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - lineHeight;
                ImGui::SameLine(rightX);
                if (ImGui::Button("X##RigidBody", ImVec2(lineHeight, lineHeight)))
                {
                    entity.RemoveComponent<RigidBodyComponent>();
                    if (opened) ImGui::TreePop();
                    return;
                }
            }

            if (opened)
            {
                auto& rb = entity.GetComponent<RigidBodyComponent>();

                // Body Type combo
                const char* bodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };
                int currentType = static_cast<int>(rb.Type);
                if (ImGui::Combo("Body Type", &currentType, bodyTypeNames, IM_ARRAYSIZE(bodyTypeNames)))
                {
                    rb.Type = static_cast<BodyType>(currentType);
                }

                // Only show mass/gravity for dynamic bodies
                if (rb.Type == BodyType::Dynamic)
                {
                    ImGui::DragFloat("Mass", &rb.Mass, 0.1f, 0.001f, 10000.0f, "%.3f");
                    ImGui::Checkbox("Use Gravity", &rb.UseGravity);
                }

                ImGui::Spacing();
                ImGui::TextUnformatted("Damping");
                ImGui::Separator();

                ImGui::DragFloat("Linear Damping", &rb.LinearDamping, 0.01f, 0.0f, 10.0f, "%.3f");
                ImGui::DragFloat("Angular Damping", &rb.AngularDamping, 0.01f, 0.0f, 10.0f, "%.3f");

                ImGui::Spacing();
                ImGui::TextUnformatted("Surface");
                ImGui::Separator();

                ImGui::DragFloat("Friction", &rb.Friction, 0.01f, 0.0f, 1.0f, "%.3f");
                ImGui::DragFloat("Restitution", &rb.Restitution, 0.01f, 0.0f, 1.0f, "%.3f");

                ImGui::TreePop();
            }
        }
    };
}
