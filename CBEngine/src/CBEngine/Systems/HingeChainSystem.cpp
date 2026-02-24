#include "cbpch.h"
#include "HingeChainSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/HingeJointComponent.h"
#include "CBEngine/Components/HingeChainComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/BlueprintAsset.h"

#include <glm/glm.hpp>

namespace CB
{
    void HingeChainSystem::DestroyChainLinks(Scene* scene, entt::entity entity, entt::registry& registry)
    {
        if (!scene) return;
        if (!registry.all_of<HingeChainComponent>(entity)) return;

        auto& chain = registry.get<HingeChainComponent>(entity);

        for (UUID linkUUID : chain.LinkEntityUUIDs)
        {
            Entity link = scene->GetEntityByUUID(linkUUID);
            if (link)
                scene->DestroyEntity(link);
        }
        chain.LinkEntityUUIDs.clear();
    }

    void HingeChainSystem::RebuildChain(Scene* scene, entt::entity e)
    {
        if (!scene) return;

        Entity rootEntity = {e, scene};
        auto& chain = rootEntity.GetComponent<HingeChainComponent>();

        // 1. Destroy old link entities
        for (UUID linkUUID : chain.LinkEntityUUIDs)
        {
            Entity link = scene->GetEntityByUUID(linkUUID);
            if (link)
                scene->DestroyEntity(link);
        }
        chain.LinkEntityUUIDs.clear();

        if (chain.LinkCount <= 0)
            return;

        // 2. Normalize chain direction
        Vector3 dir = chain.ChainDirection;
        float len = glm::length(dir);
        if (len < 1e-6f)
            dir = Vector3(0.0f, -1.0f, 0.0f);
        else
            dir /= len;

        UUID rootUUID = rootEntity.GetUUID();

        // 3. Load blueprint asset if needed
        Ref<BlueprintAsset> blueprintAsset;
        if (chain.LinkType == ChainLinkType::Blueprint)
        {
            if (!chain.BlueprintUUID.IsValid())
            {
                CB_CORE_WARN("[HingeChain] Blueprint mode but no BlueprintUUID set");
                return;
            }
            blueprintAsset = AssetManager::GetAsset<BlueprintAsset>(chain.BlueprintUUID);
            if (!blueprintAsset || blueprintAsset->YAMLData.empty())
            {
                CB_CORE_WARN("[HingeChain] Blueprint asset not found or empty");
                return;
            }
        }

        // 4. Create link entities
        std::vector<UUID> createdUUIDs;
        createdUUIDs.reserve(chain.LinkCount);

        // Non-owning shared_ptr to scene — used by SceneSerializer::InstantiateBlueprint
        Ref<Scene> sceneRef(scene, [](Scene*) {});

        for (int i = 0; i < chain.LinkCount; ++i)
        {
            Entity linkEntity;

            if (chain.LinkType == ChainLinkType::Mesh)
            {
                // ── Mesh mode ─────────────────────────────────────────────────
                std::string name = "ChainLink_" + std::to_string(i);
                linkEntity = scene->CreateEntity(name);

                auto& transform = linkEntity.GetComponent<TransformComponent>();
                transform.Position = dir * (chain.LinkSpacing * (static_cast<float>(i) + 0.5f));
                transform.Scale    = chain.LinkScale;
                transform.Parent   = rootUUID;
                rootEntity.GetComponent<TransformComponent>().Children.push_back(linkEntity.GetUUID());

                auto& mr = linkEntity.AddComponent<MeshRendererComponent>();
                mr.MeshUUID     = chain.MeshUUID;
                mr.MaterialUUID = chain.MaterialUUID;
                mr.ResolveAssets();

                auto& rb = linkEntity.AddComponent<RigidBodyComponent>();
                rb.Type           = BodyType::Dynamic;
                rb.Mass           = chain.LinkMass;
                rb.LinearDamping  = chain.LinearDamping;
                rb.AngularDamping = chain.AngularDamping;
                rb.UseGravity     = true;

                auto& col = linkEntity.AddComponent<ColliderComponent>();
                col.Shape       = ColliderShape::Box;
                col.HalfExtents = chain.ColliderHalfExtents;
            }
            else
            {
                // ── Blueprint mode ─────────────────────────────────────────────
                SceneSerializer serializer(sceneRef);
                std::vector<Entity> spawned = serializer.InstantiateBlueprint(
                    blueprintAsset->YAMLData, "", chain.BlueprintUUID);

                if (spawned.empty())
                {
                    CB_CORE_WARN("[HingeChain] Blueprint instantiation returned no entities for link {0}", i);
                    continue;
                }

                // Root of the instantiated hierarchy
                linkEntity = spawned[0];

                // Position and parent — same convention as mesh mode
                auto& transform = linkEntity.GetComponent<TransformComponent>();
                transform.Position = dir * (chain.LinkSpacing * (static_cast<float>(i) + 0.5f));
                // Do NOT touch Scale or RigidBody — Blueprint owns those
                transform.Parent = rootUUID;
                rootEntity.GetComponent<TransformComponent>().Children.push_back(linkEntity.GetUUID());
            }

            // ── HingeJoint — applied in both modes ────────────────────────────
            // If the Blueprint already has a HingeJointComponent, overwrite it;
            // otherwise add one. The chain system owns the connectivity.
            if (linkEntity.HasComponent<HingeJointComponent>())
                linkEntity.GetComponent<HingeJointComponent>() = HingeJointComponent{};
            else
                linkEntity.AddComponent<HingeJointComponent>();

            auto& hj = linkEntity.GetComponent<HingeJointComponent>();
            hj.Anchor    = -dir * (chain.LinkSpacing * 0.5f);
            hj.Axis      = chain.HingeAxis;
            hj.AutoConfigureConnectedAnchor = true;

            hj.UseLimits       = chain.UseLimits;
            hj.LimitsMin       = chain.LimitsMin;
            hj.LimitsMax       = chain.LimitsMax;
            hj.Bounciness      = chain.Bounciness;

            hj.UseSpring       = chain.UseSpring;
            hj.SpringFrequency = chain.SpringFrequency;
            hj.SpringDamping   = chain.SpringDamping;

            hj.UseMotor        = chain.UseMotor;
            hj.TargetVelocity  = chain.TargetVelocity;
            hj.MotorForce      = chain.MotorForce;

            hj.BreakForce      = chain.BreakForce;
            hj.BreakTorque     = chain.BreakTorque;

            hj.EnableCollision   = chain.EnableCollisionBetweenLinks;
            hj.MaxFrictionTorque = chain.MaxFrictionTorque;

            hj.ConnectedBodyUUID = (i == 0)
                ? (chain.AnchorFirstToWorld ? UUID(0)
                    : (rootEntity.HasComponent<RigidBodyComponent>() ? rootUUID : UUID(0)))
                : createdUUIDs[i - 1];

            hj.ConstraintCreated = false;

            createdUUIDs.push_back(linkEntity.GetUUID());
        }

        chain.LinkEntityUUIDs = std::move(createdUUIDs);

        CB_CORE_INFO("[HingeChain] Rebuilt chain ({0}) with {1} links",
            chain.LinkType == ChainLinkType::Blueprint ? "Blueprint" : "Mesh",
            chain.LinkCount);
    }
}
