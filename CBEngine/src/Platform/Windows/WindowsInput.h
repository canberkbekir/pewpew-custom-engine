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
        void FeedKeyPressImpl(int key) override;
        void FeedKeyReleaseImpl(int key) override;
        void FeedMousePressImpl(int button) override;
        void FeedMouseReleaseImpl(int button) override;

    private:
        // Key state snapshot for IsKeyPressed (held-down queries)
        std::array<bool, 512> m_PrevKeyStates{};
        // Mouse button state snapshot (held-down queries)
        std::array<bool, 8> m_PrevMouseButtonStates{};

        // Callback-fed just-pressed/released accumulators (written during glfwPollEvents)
        std::array<bool, 512> m_AccumJustPressed{};
        std::array<bool, 512> m_AccumJustReleased{};
        std::array<bool, 8>   m_AccumMouseJustPressed{};
        std::array<bool, 8>   m_AccumMouseJustReleased{};

        // Per-frame snapshot of the above (read by IsKeyJustPressed/Released during Lua update)
        std::array<bool, 512> m_FrameJustPressed{};
        std::array<bool, 512> m_FrameJustReleased{};
        std::array<bool, 8>   m_FrameMouseJustPressed{};
        std::array<bool, 8>   m_FrameMouseJustReleased{};

        // Previous mouse position for per-frame delta computation
        float m_PrevMouseX = 0.0f;
        float m_PrevMouseY = 0.0f;
        bool m_FirstFrame = true;

        // Snapshotted mouse delta for the current frame (computed in UpdateImpl, read by GetMouseDeltaImpl)
        float m_FrameMouseDeltaX = 0.0f;
        float m_FrameMouseDeltaY = 0.0f;

        // Scroll accumulator — fed by the window scroll callback, consumed each UpdateImpl
        float m_AccumScrollX = 0.0f;
        float m_AccumScrollY = 0.0f;
        // Snapshotted scroll delta for the current frame (read by GetMouseScrollDeltaImpl)
        float m_FrameScrollX = 0.0f;
        float m_FrameScrollY = 0.0f;

        bool m_CursorLocked = false;
    };
}
