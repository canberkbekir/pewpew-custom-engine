#include "cbpch.h"
#include "HingeJointSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/HingeJointComponent.h"
#include "CBEngine/Physics/PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SpringSettings.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>

namespace CB
{
    static JPH::Vec3 ToJolt(const Vector3& v)
    {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    void HingeJointSystem::OnUpdate(Scene* scene, Timestep ts)
    {
        if (!scene) return;
        PhysicsWorld* world = scene->GetPhysicsWorld();
        if (!world)  return;

        auto& registry = scene->GetRegistry();
        auto view = registry.view<HingeJointComponent>();

        for (auto e : view)
        {
            auto& hj = view.get<HingeJointComponent>(e);

            if (hj.IsBroken)
                continue;

            // Lazy init
            if (!hj.ConstraintCreated)
                InitConstraint(scene, e);

            if (!hj.ConstraintCreated || !hj.RuntimeConstraint)
                continue;

            auto* constraint = static_cast<JPH::HingeConstraint*>(hj.RuntimeConstraint);

            // Break check
            bool shouldBreak = false;
            if (hj.BreakForce > 0.0f || hj.BreakTorque > 0.0f)
            {
                JPH::Vec3 totalLambdaPos = constraint->GetTotalLambdaPosition();
                float linearForce = totalLambdaPos.Length() / ts.GetSeconds();

                float totalLambdaRot = std::abs(constraint->GetTotalLambdaRotationLimits())
                    + std::abs(constraint->GetTotalLambdaMotor());
                float angularTorque = totalLambdaRot / ts.GetSeconds();

                if ((hj.BreakForce  > 0.0f && linearForce   > hj.BreakForce)  ||
                    (hj.BreakTorque > 0.0f && angularTorque > hj.BreakTorque))
                {
                    shouldBreak = true;
                }
            }

            if (shouldBreak)
            {
                world->GetPhysicsSystem().RemoveConstraint(hj.RuntimeConstraint);
                hj.RuntimeConstraint->Release();
                hj.RuntimeConstraint = nullptr;
                hj.ConstraintCreated = false;
                hj.IsBroken          = true;
                CB_CORE_INFO("[HingeJoint] Constraint broken on entity");
                continue;
            }

            // Motor velocity update
            if (hj.UseMotor)
            {
                constraint->SetTargetAngularVelocity(glm::radians(hj.TargetVelocity));
            }
        }
    }

    void HingeJointSystem::InitConstraint(Scene* scene, entt::entity e)
    {
        if (!scene) return;
        PhysicsWorld* world = scene->GetPhysicsWorld();
        if (!world)  return;

        auto& registry = scene->GetRegistry();

        if (!registry.all_of<HingeJointComponent, TransformComponent, RigidBodyComponent>(e))
            return;

        auto& hj        = registry.get<HingeJointComponent>(e);
        auto& transform = registry.get<TransformComponent>(e);
        auto& rb        = registry.get<RigidBodyComponent>(e);

        if (!rb.BodyCreated)
            return;

        // -- Compute world-space anchor and axis for body1 --
        const Mat4& world1 = transform.WorldMatrix;

        // World-space anchor point: apply entity's WorldMatrix to local Anchor
        Vector3 worldAnchor1 = Vector3(world1 * glm::vec4(hj.Anchor, 1.0f));

        // World-space hinge axis (rotate-only): upper-left 3x3 of WorldMatrix
        // We normalize it for safety
        Vector3 worldAxis = glm::normalize(Vector3(world1 * glm::vec4(hj.Axis, 0.0f)));
        if (glm::length2(worldAxis) < 1e-8f)
            worldAxis = Vector3(0.0f, 1.0f, 0.0f);

        // Build a perpendicular normal axis (any vector perpendicular to hinge axis)
        Vector3 worldNormal = std::abs(worldAxis.x) < 0.9f
            ? Vector3(1.0f, 0.0f, 0.0f)
            : Vector3(0.0f, 1.0f, 0.0f);
        worldNormal = glm::normalize(glm::cross(worldAxis, worldNormal));

        // -- Determine anchor for body2 --
        Vector3 worldAnchor2 = worldAnchor1; // default: same world point

        Entity connectedEntity;
        if (hj.ConnectedBodyUUID != UUID(0))
        {
            connectedEntity = scene->GetEntityByUUID(hj.ConnectedBodyUUID);
            if (!connectedEntity)
            {
                // Connected body was destroyed (e.g. a chain link was shot).
                // Mark as broken so this body falls freely instead of snapping to world.
                hj.IsBroken          = true;
                hj.ConstraintCreated = true; // stop retrying every frame
                return;
            }
        }

        if (connectedEntity && connectedEntity.HasComponent<TransformComponent>())
        {
            const Mat4& world2 = connectedEntity.GetComponent<TransformComponent>().WorldMatrix;

            if (hj.AutoConfigureConnectedAnchor)
            {
                // Auto: use same world-space point
                worldAnchor2 = worldAnchor1;
            }
            else
            {
                // Manual: transform ConnectedAnchor from connected body's local space
                worldAnchor2 = Vector3(world2 * glm::vec4(hj.ConnectedAnchor, 1.0f));
            }
        }

        // -- Fill Jolt constraint settings (WorldSpace) --
        JPH::HingeConstraintSettings settings;
        settings.mSpace       = JPH::EConstraintSpace::WorldSpace;
        settings.mPoint1      = JPH::RVec3(worldAnchor1.x, worldAnchor1.y, worldAnchor1.z);
        settings.mPoint2      = JPH::RVec3(worldAnchor2.x, worldAnchor2.y, worldAnchor2.z);
        settings.mHingeAxis1  = ToJolt(worldAxis);
        settings.mHingeAxis2  = ToJolt(worldAxis);
        settings.mNormalAxis1 = ToJolt(worldNormal);
        settings.mNormalAxis2 = ToJolt(worldNormal);

        // Limits
        if (hj.UseLimits)
        {
            settings.mLimitsMin = glm::radians(glm::clamp(hj.LimitsMin, -180.0f, 0.0f));
            settings.mLimitsMax = glm::radians(glm::clamp(hj.LimitsMax,  0.0f, 180.0f));
        }
        else
        {
            settings.mLimitsMin = -glm::pi<float>();
            settings.mLimitsMax =  glm::pi<float>();
        }

        // Spring (soft limits)
        if (hj.UseSpring)
        {
            settings.mLimitsSpringSettings.mMode      = JPH::ESpringMode::FrequencyAndDamping;
            settings.mLimitsSpringSettings.mFrequency = hj.SpringFrequency;
            settings.mLimitsSpringSettings.mDamping   = hj.SpringDamping;
        }

        // Motor
        if (hj.UseMotor)
        {
            settings.mMotorSettings = JPH::MotorSettings(2.0f, 1.0f, FLT_MAX, hj.MotorForce);
        }

        settings.mMaxFrictionTorque = hj.MaxFrictionTorque;

        // -- Acquire body references --
        JPH::PhysicsSystem& physSystem = world->GetPhysicsSystem();
        // Use the no-lock interface to avoid Jolt's same-priority lock assert.
        // InitConstraint is called from HingeJointSystem::OnUpdate (priority 350),
        // which runs after PhysicsSystem (300) — no physics step is in progress,
        // so no simulation locks are held and no-lock access is safe.
        const JPH::BodyLockInterface& lockInterface = physSystem.GetBodyLockInterfaceNoLock();

        JPH::TwoBodyConstraint* constraint = nullptr;

        if (connectedEntity && connectedEntity.HasComponent<RigidBodyComponent>())
        {
            auto& rb2 = connectedEntity.GetComponent<RigidBodyComponent>();
            if (!rb2.BodyCreated)
                return;

            JPH::BodyLockWrite lock1(lockInterface, rb.RuntimeBodyID);
            JPH::BodyLockWrite lock2(lockInterface, rb2.RuntimeBodyID);

            if (!lock1.Succeeded() || !lock2.Succeeded())
            {
                CB_CORE_WARN("[HingeJoint] Failed to lock bodies for constraint creation");
                return;
            }

            JPH::Body& body1 = lock1.GetBody();
            JPH::Body& body2 = lock2.GetBody();

            constraint = static_cast<JPH::TwoBodyConstraint*>(settings.Create(body1, body2));
        }
        else
        {
            // Attach to world (sFixedToWorld) — single body, no ordering issue,
            // but use the same no-lock interface for consistency.
            JPH::BodyLockWrite lock1(lockInterface, rb.RuntimeBodyID);
            if (!lock1.Succeeded())
            {
                CB_CORE_WARN("[HingeJoint] Failed to lock body for world constraint");
                return;
            }

            JPH::Body& body1 = lock1.GetBody();
            constraint = static_cast<JPH::TwoBodyConstraint*>(
                settings.Create(body1, JPH::Body::sFixedToWorld));
        }

        if (!constraint)
        {
            CB_CORE_ERROR("[HingeJoint] Constraint creation returned nullptr");
            return;
        }

        // Motor state
        if (hj.UseMotor)
        {
            auto* hingeConstraint = static_cast<JPH::HingeConstraint*>(constraint);
            hingeConstraint->SetMotorState(JPH::EMotorState::Velocity);
            hingeConstraint->SetTargetAngularVelocity(glm::radians(hj.TargetVelocity));
        }

        // AddRef so we own a reference (physics system adds its own ref via AddConstraint)
        constraint->AddRef();

        physSystem.AddConstraint(constraint);

        hj.RuntimeConstraint = constraint;
        hj.ConstraintCreated = true;
        hj.IsBroken          = false;

        CB_CORE_TRACE("[HingeJoint] Constraint created for entity");
    }

    void HingeJointSystem::DestroyConstraint(PhysicsWorld* world, entt::entity e, entt::registry& registry)
    {
        if (!world) return;

        // 1. Remove this entity's own constraint (this link → its parent).
        if (registry.all_of<HingeJointComponent>(e))
        {
            auto& hj = registry.get<HingeJointComponent>(e);
            if (hj.ConstraintCreated && hj.RuntimeConstraint)
            {
                world->GetPhysicsSystem().RemoveConstraint(hj.RuntimeConstraint);
                hj.RuntimeConstraint->Release();
                hj.RuntimeConstraint = nullptr;
                hj.ConstraintCreated = false;
            }
        }

        // 2. Remove any constraint whose ConnectedBodyUUID points TO this entity
        //    (i.e. the link directly below in a chain). This must happen before the
        //    body is destroyed so Jolt never holds a dangling body reference.
        //    The downstream link's ConstraintCreated is cleared so InitConstraint
        //    retries next frame — at that point the entity won't exist and the link
        //    will be marked IsBroken (free body) rather than re-attached to the world.
        if (!registry.all_of<IDComponent>(e)) return;
        const UUID destroyedUUID = registry.get<IDComponent>(e).ID;

        auto view = registry.view<HingeJointComponent>();
        for (auto other : view)
        {
            if (other == e) continue;
            auto& otherHJ = view.get<HingeJointComponent>(other);
            if (otherHJ.ConnectedBodyUUID == destroyedUUID
                && otherHJ.ConstraintCreated && otherHJ.RuntimeConstraint)
            {
                world->GetPhysicsSystem().RemoveConstraint(otherHJ.RuntimeConstraint);
                otherHJ.RuntimeConstraint->Release();
                otherHJ.RuntimeConstraint = nullptr;
                otherHJ.ConstraintCreated = false;
                // ConnectedBodyUUID intentionally kept — InitConstraint will see a
                // non-zero UUID with no matching entity and set IsBroken = true.
            }
        }
    }
}
