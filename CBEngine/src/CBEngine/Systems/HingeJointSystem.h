#pragma once

#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Scene/Scene.h"

namespace CB
{
    class Scene;
    class PhysicsWorld;

    class HingeJointSystem
    {
    public:
        static void OnUpdate(Scene* scene, Timestep ts);
        static void InitConstraint(Scene* scene, entt::entity entity);
        static void DestroyConstraint(PhysicsWorld* world, entt::entity entity, entt::registry& registry);
    };
}
