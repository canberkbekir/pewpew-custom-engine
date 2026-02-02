#pragma once

#include "Panel.h"

namespace PewPew
{
    class PropertiesPanel : public Panel
    {
    public:
        PropertiesPanel()
            : Panel("Properties", true) {}

        void OnImGuiRender() override;
    };
}
