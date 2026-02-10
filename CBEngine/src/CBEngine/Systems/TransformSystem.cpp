#include "cbpch.h"
#include "TransformSystem.h"

#include "CBEngine/Scene/Entity.h"

namespace CB
{
    void TransformSystem::OnUpdate(Scene* scene, Timestep ts)
    {
        auto view = scene->GetRegistry().view<TransformComponent>();

        for (auto entity : view)
        {
            auto& transform = view.get<TransformComponent>(entity);
            if (transform.Parent == 0) // Root entity
            {
                UpdateHierarchy(scene, Entity{entity, scene}, Mat4(1.0f));
            }
        }
    }

    void TransformSystem::UpdateHierarchy(Scene* scene, Entity entity, const Mat4& parentWorld)
    {
        auto& transform = entity.GetComponent<TransformComponent>();

        // World = Parent * Local
        transform.WorldMatrix = parentWorld * transform.GetLocalTransform();
        transform.Dirty = false;

        // Update children
        for (UUID childUUID : transform.Children)
        {
            if (Entity child = scene->GetEntityByUUID(childUUID))
            {
                UpdateHierarchy(scene, child, transform.WorldMatrix);
            }
        }
    }
}
