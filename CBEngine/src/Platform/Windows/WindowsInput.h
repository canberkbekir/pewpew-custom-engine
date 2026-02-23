#pragma once
#include "CBEngine/Input/Input.h"
#include <array>

namespace CB
{
    class WindowsInput : public Input
    {
    public:
        ~WindowsInput() override = default;

    protected:
        void UpdateImpl() override;
        bool IsKeyPressedImpl(int keycode) override;
        bool IsKeyJustPressedImpl(int keycode) override;
        bool IsKeyJustReleasedImpl(int keycode) override;
        bool IsMouseButtonPressedImpl(int button) override;
        std::pair<float, float> GetMousePositionImpl() override;
        float GetMouseXImpl() override;
        float GetMouseYImpl() override;
        std::pair<float, float> GetMouseDeltaImpl() override;
        std::pair<float, float> GetMouseScrollDeltaImpl() override;
        void SetCursorLockedImpl(bool locked) override;
        bool IsCursorLockedImpl() const override { return m_CursorLocked; }
        void FeedScrollImpl(float x, float y) override;

    private:
        // Key state snapshot for JustPressed/JustReleased detection
        std::array<bool, 512> m_PrevKeyStates{};
        // Mouse button state snapshot for unified key JustPressed/JustReleased
        std::array<bool, 8> m_PrevMouseButtonStates{};

        // Previous mouse position for per-frame delta computation
        float m_PrevMouseX = 0.0f;
        float m_PrevMouseY = 0.0f;
        bool m_FirstFrame = true;

        // Scroll accumulator — fed by the window scroll callback, consumed each UpdateImpl
        float m_AccumScrollX = 0.0f;
        float m_AccumScrollY = 0.0f;
        // Snapshotted scroll delta for the current frame (read by GetMouseScrollDeltaImpl)
        float m_FrameScrollX = 0.0f;
        float m_FrameScrollY = 0.0f;

        bool m_CursorLocked = false;
    };
}
