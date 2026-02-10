#pragma once

#include "Event.h"

namespace CB
{
    class PlayModeEnterEvent : public Event
    {
    public:
        PlayModeEnterEvent() = default;

        std::string ToString() const override
        {
            return "PlayModeEnterEvent";
        }

        EVENT_CLASS_TYPE(PlayModeEnter)
        EVENT_CLASS_CATEGORY(EventCategoryEditor)
    };

    class PlayModeExitEvent : public Event
    {
    public:
        PlayModeExitEvent() = default;

        std::string ToString() const override
        {
            return "PlayModeExitEvent";
        }

        EVENT_CLASS_TYPE(PlayModeExit)
        EVENT_CLASS_CATEGORY(EventCategoryEditor)
    };
}
