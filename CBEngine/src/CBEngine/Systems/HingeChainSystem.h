#pragma once

#include "CBEngine/Scene/Scene.h"

namespace CB
{
    class Scene;

    // Pure utility class — no ISystem, no per-frame polling.
    // Rebuild is triggered explicitly by the editor; cleanup is triggered by
    // the on_destroy<HingeChainComponent> signal wired in Scene::Scene().
    class HingeChainSystem
    {
    public:
        static void RebuildChain(Scene* scene, entt::entity entity);
        static void DestroyChainLinks(Scene* scene, entt::entity entity, entt::registry& registry);
    };
}
