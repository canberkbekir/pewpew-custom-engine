#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Components/HingeChainComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Systems/HingeChainSystem.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Asset/BlueprintAsset.h"
#include "../ComponentCard.h"
#include "../AssetPicker.h"

#include <cfloat>

namespace CB
{
    class HingeChainComponentView
    {
    public:
        static void Draw(Entity entity)
        {
            if (!entity.HasComponent<HingeChainComponent>())
                return;

            bool removed = false;
            bool reset   = false;

            bool opened = ComponentCard::Begin("Hinge Chain", true, &removed, &reset);

            if (removed)
            {
                // Removing the component fires on_destroy<HingeChainComponent>, which
                // calls HingeChainSystem::DestroyChainLinks via the registered signal.
                entity.RemoveComponent<HingeChainComponent>();
                ComponentCard::End();
                return;
            }

            if (reset)
            {
                Scene* scene = entity.GetScene();
                if (scene)
                    HingeChainSystem::DestroyChainLinks(scene, entity, scene->GetRegistry());
                entity.GetComponent<HingeChainComponent>() = HingeChainComponent{};
                HingeChainSystem::RebuildChain(entity.GetScene(), entity);
            }

            if (!opened)
            {
                ComponentCard::End();
                return;
            }

            auto& chain = entity.GetComponent<HingeChainComponent>();

            // Accumulated rebuild request — set by checkboxes (immediate) or by
            // IsItemDeactivatedAfterEdit (drag widgets, so we don't rebuild 60x/sec).
            bool shouldRebuild = false;

            // ---- Chain Structure ----
            ImGui::TextUnformatted("Chain Structure");
            ImGui::Separator();

            {
                int lc = chain.LinkCount;
                if (ImGui::DragInt("Link Count##HC", &lc, 1, 1, 256))
                    chain.LinkCount = glm::max(lc, 1);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;
            }
            {
                float ls = chain.LinkSpacing;
                if (ImGui::DragFloat("Link Spacing##HC", &ls, 0.01f, 0.01f, 100.0f))
                    chain.LinkSpacing = glm::max(ls, 0.01f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;
            }
            ImGui::DragFloat3("Chain Direction##HC", &chain.ChainDirection.x, 0.01f);
            if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;

            ImGui::Spacing();

            // ---- Link Type dropdown ----
            ImGui::TextUnformatted("Link Type");
            ImGui::Separator();

            {
                const char* linkTypeNames[] = { "Mesh", "Blueprint" };
                int currentType = static_cast<int>(chain.LinkType);
                if (ImGui::Combo("Type##HC", &currentType, linkTypeNames, 2))
                {
                    chain.LinkType = static_cast<ChainLinkType>(currentType);
                    shouldRebuild = true;
                }
            }

            ImGui::Spacing();

            if (chain.LinkType == ChainLinkType::Mesh)
            {
                // ---- Mesh mode: Rendering ----
                ImGui::TextUnformatted("Rendering");
                ImGui::Separator();

                if (AssetPicker::DrawMesh("Mesh##HC", chain.MeshUUID))
                {
                    if (chain.MeshUUID.IsValid())
                    {
                        auto pmAsset = AssetManager::GetAsset<ProcessedMeshAsset>(chain.MeshUUID);
                        if (pmAsset && pmAsset->HasGeometryData())
                        {
                            Vector3 bMin(FLT_MAX), bMax(-FLT_MAX);
                            for (const auto& v : pmAsset->Vertices)
                            {
                                bMin = glm::min(bMin, v.Position);
                                bMax = glm::max(bMax, v.Position);
                            }
                            if (bMin.x < FLT_MAX)
                                chain.ColliderHalfExtents = glm::max((bMax - bMin) * 0.5f, Vector3(0.01f));
                        }
                    }
                    shouldRebuild = true;
                }
                if (AssetPicker::DrawMaterial("Material##HC", chain.MaterialUUID)) shouldRebuild = true;

                ImGui::DragFloat3("Link Scale##HC", &chain.LinkScale.x, 0.01f, 0.001f, 1000.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;

                ImGui::Spacing();

                // ---- Mesh mode: Per-Link Physics ----
                ImGui::TextUnformatted("Per-Link Physics");
                ImGui::Separator();

                ImGui::DragFloat3("Collider Half Extents##HC", &chain.ColliderHalfExtents.x, 0.01f, 0.01f, 100.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;

                ImGui::DragFloat("Link Mass (kg)##HC", &chain.LinkMass, 0.1f, 0.01f, 1e6f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;

                ImGui::DragFloat("Linear Damping##HC",  &chain.LinearDamping,  0.001f, 0.0f, 1.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;

                ImGui::DragFloat("Angular Damping##HC", &chain.AngularDamping, 0.001f, 0.0f, 1.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;

                ImGui::Spacing();
            }
            else
            {
                // ---- Blueprint mode ----
                ImGui::TextUnformatted("Blueprint");
                ImGui::Separator();

                if (AssetPicker::Draw("Blueprint##HC", chain.BlueprintUUID, AssetType::Blueprint))
                    shouldRebuild = true;

                if (chain.BlueprintUUID.IsValid())
                {
                    auto bp = AssetManager::GetAsset<BlueprintAsset>(chain.BlueprintUUID);
                    if (bp)
                        ImGui::TextDisabled("  %s  (%d entities)", bp->RootEntityName.c_str(), bp->EntityCount);
                }

                ImGui::Spacing();
            }

            // ---- Anchor (both modes) ----
            if (ImGui::Checkbox("Anchor First Link to World##HC", &chain.AnchorFirstToWorld))
                shouldRebuild = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("If false, first link attaches to this entity's Rigid Body");

            ImGui::Spacing();

            // ---- Hinge Settings ----
            ImGui::TextUnformatted("Hinge Settings");
            ImGui::Separator();

            ImGui::DragFloat3("Hinge Axis##HC", &chain.HingeAxis.x, 0.01f);
            if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;

            if (ImGui::Checkbox("Use Limits##HC", &chain.UseLimits)) shouldRebuild = true;
            if (chain.UseLimits)
            {
                ImGui::DragFloat("Min (deg)##HC", &chain.LimitsMin, 1.0f, -180.0f, 0.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;
                ImGui::DragFloat("Max (deg)##HC", &chain.LimitsMax, 1.0f, 0.0f, 180.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;
                ImGui::DragFloat("Bounciness##HC", &chain.Bounciness, 0.01f, 0.0f, 1.0f);
            }

            if (ImGui::Checkbox("Use Spring##HC", &chain.UseSpring)) shouldRebuild = true;
            if (chain.UseSpring)
            {
                ImGui::DragFloat("Frequency (Hz)##HC", &chain.SpringFrequency, 0.1f, 0.0f, 100.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;
                ImGui::DragFloat("Damping##HC", &chain.SpringDamping, 0.01f, 0.0f, 10.0f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;
            }

            if (ImGui::Checkbox("Use Motor##HC", &chain.UseMotor)) shouldRebuild = true;
            if (chain.UseMotor)
            {
                ImGui::DragFloat("Target Velocity (deg/s)##HC", &chain.TargetVelocity, 1.0f);
                ImGui::DragFloat("Motor Force (Nm)##HC", &chain.MotorForce, 10.0f, 0.0f, 1e8f);
                if (ImGui::IsItemDeactivatedAfterEdit()) shouldRebuild = true;
            }

            ImGui::Spacing();

            // ---- Break ----
            ImGui::TextUnformatted("Break");
            ImGui::Separator();

            ImGui::DragFloat("Break Force (N)##HC",  &chain.BreakForce,  1.0f, 0.0f, 1e8f);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = unbreakable");
            ImGui::DragFloat("Break Torque (Nm)##HC",&chain.BreakTorque, 1.0f, 0.0f, 1e8f);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = unbreakable");

            ImGui::Spacing();

            // ---- Options ----
            ImGui::TextUnformatted("Options");
            ImGui::Separator();

            if (ImGui::Checkbox("Enable Collision Between Links##HC", &chain.EnableCollisionBetweenLinks))
                shouldRebuild = true;
            ImGui::DragFloat("Max Friction Torque (Nm)##HC", &chain.MaxFrictionTorque, 0.1f, 0.0f, 1e6f);

            ImGui::Spacing();

            // ---- Runtime ----
            ImGui::TextUnformatted("Runtime");
            ImGui::Separator();

            ImGui::Text("Links: %d / %d",
                static_cast<int>(chain.LinkEntityUUIDs.size()), chain.LinkCount);

            if (ImGui::Button("Rebuild Chain##HC"))
                shouldRebuild = true;

            // Commit rebuild once per frame (avoids multiple rebuilds from a single edit)
            if (shouldRebuild)
                HingeChainSystem::RebuildChain(entity.GetScene(), entity);

            ComponentCard::End();
        }

    };
}
