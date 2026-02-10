#pragma once

#include "Panel.h"

namespace CB
{
    class ConsolePanel : public Panel
    {
    public:
        ConsolePanel()
            : Panel("Console", true)
        {
        }

        void OnImGuiRender() override;

    private:
        bool m_AutoScroll = true;
        bool m_ShowCore = true;
        bool m_ShowClient = true;

        // Filter by level
        bool m_ShowTrace = true;
        bool m_ShowInfo = true;
        bool m_ShowWarn = true;
        bool m_ShowError = true;
    };
}
