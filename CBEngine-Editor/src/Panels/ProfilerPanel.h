#pragma once

#include "Panel.h"
#include "CBEngine/Events/KeyEvent.h"
#include "CBEngine/Input/KeyCodes.h"

namespace CB
{
    class ProfilerPanel : public Panel
    {
    public:
        ProfilerPanel()
            : Panel("Profiler", false)
        {
        }

        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        bool OnKeyPressed(KeyPressedEvent& e);
        void RenderScopeTree();
    };
}
