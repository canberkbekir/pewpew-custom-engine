#pragma once
#include "PewPew/Input.h"

namespace PewPew
{
    class WindowsInput : public Input
    {
    public:
        ~WindowsInput() override = default;

    protected:
        bool IsKeyPressedImpl(int keycode) override;

        bool IsMouseButtonPressedImpl(int button) override;
        std::pair<float, float> GetMousePositionImpl() override;
        float GetMouseXImpl() override;
        float GetMouseYImpl() override;
    };
}
