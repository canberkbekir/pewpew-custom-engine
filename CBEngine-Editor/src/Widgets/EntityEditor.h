#pragma once

#include "imgui.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/SceneManager.h"

// Component views
#include "ComponentViews/TagComponentView.h"
#include "ComponentViews/TransformComponentView.h"
#include "ComponentViews/DirectionalLightComponentView.h"
#include "ComponentViews/MeshRendererComponentView.h"
#include "ComponentViews/VoxelRendererComponentView.h"
#include "ComponentViews/RigidBodyComponentView.h"
#include "ComponentViews/ColliderComponentView.h"
#include "ComponentViews/AddComponentMenu.h"

namespace CB
{
    class EntityEditor
    {
    public:
        static void Draw(UUID entityUUID)
        {
            Ref<Scene> scene = SceneManager::GetActiveScene();
            if (!scene)
            {
                ImGui::TextDisabled("No active scene");
                return;
            }

            Entity entity = scene->GetEntityByUUID(entityUUID);
            if (!entity)
            {
                ImGui::TextDisabled("Entity not found");
                return;
            }

            Draw(entity);
        }

        static void Draw(Entity entity)
        {
            if (!entity)
            {
                ImGui::TextDisabled("Invalid entity");
                return;
            }

            TagComponentView::Draw(entity);

            ImGui::Separator();

            TransformComponentView::Draw(entity);
            DirectionalLightComponentView::Draw(entity);
            MeshRendererComponentView::Draw(entity);
            VoxelRendererComponentView::Draw(entity);
            RigidBodyComponentView::Draw(entity);
            ColliderComponentView::Draw(entity);

            ImGui::Separator();

            AddComponentMenu::Draw(entity);
        }
    };
}
