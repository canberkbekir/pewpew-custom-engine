#pragma once

#include "Panel.h"

namespace PewPew
{
    class SceneHierarchyPanel : public Panel
    {
    public:
        SceneHierarchyPanel()
            : Panel("Scene Hierarchy", true) {}

        void OnImGuiRender() override;
    };
}
