#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "../ComponentCard.h"

namespace CB
{
    class RigidBodyComponentView
    {
    public:
        static void Draw(Entity entity)
        {
            if (!entity.HasComponent<RigidBodyComponent>())
                return;

            bool removed = false;
            bool reset = false;

            bool opened = ComponentCard::Begin("Rigid Body", true, &removed, &reset);

            if (removed)
            {
                entity.RemoveComponent<RigidBodyComponent>();
                ComponentCard::End();
                return;
            }

            if (reset)
            {
                auto& rb = entity.GetComponent<RigidBodyComponent>();
                rb.Type = BodyType::Static;
                rb.Mass = 1.0f;
                rb.UseGravity = true;
                rb.LinearDamping = 0.0f;
                rb.AngularDamping = 0.05f;
                rb.Friction = 0.2f;
                rb.Restitution = 0.0f;
                rb.FreezePositionX = rb.FreezePositionY = rb.FreezePositionZ = false;
                rb.FreezeRotationX = rb.FreezeRotationY = rb.FreezeRotationZ = false;
            }

            if (opened)
            {
                auto& rb = entity.GetComponent<RigidBodyComponent>();

                // Body Type combo
                const char* bodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };
                int currentType = static_cast<int>(rb.Type);
                if (ImGui::Combo("Body Type", &currentType, bodyTypeNames, IM_ARRAYSIZE(bodyTypeNames)))
                    rb.Type = static_cast<BodyType>(currentType);

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

                // Freeze constraints (only relevant for Dynamic / Kinematic)
                if (rb.Type != BodyType::Static)
                {
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Constraints");
                    ImGui::Separator();
                    ImGui::TextDisabled("Takes effect when play mode starts");

                    // Freeze Position row
                    ImGui::TextUnformatted("Freeze Position");
                    ImGui::SameLine(120.0f);
                    ImGui::Checkbox("X##FPX", &rb.FreezePositionX);
                    ImGui::SameLine();
                    ImGui::Checkbox("Y##FPY", &rb.FreezePositionY);
                    ImGui::SameLine();
                    ImGui::Checkbox("Z##FPZ", &rb.FreezePositionZ);

                    // Freeze Rotation row
                    ImGui::TextUnformatted("Freeze Rotation");
                    ImGui::SameLine(120.0f);
                    ImGui::Checkbox("X##FRX", &rb.FreezeRotationX);
                    ImGui::SameLine();
                    ImGui::Checkbox("Y##FRY", &rb.FreezeRotationY);
                    ImGui::SameLine();
                    ImGui::Checkbox("Z##FRZ", &rb.FreezeRotationZ);
                }
            }

            ComponentCard::End();
        }
    };
}
