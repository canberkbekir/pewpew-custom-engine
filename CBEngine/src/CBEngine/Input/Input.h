#pragma once
#include "CBEngine/Core/Core.h"

namespace CB
{
    class Input
    {
    protected:
        Input() = default;

    public:
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;
        virtual ~Input() = default;

        // Called once per frame before layer updates to snapshot previous state
        static void Update() { s_Instance->UpdateImpl(); }

        static bool IsKeyPressed(int keycode) { return s_Instance->IsKeyPressedImpl(keycode); }
        // Returns true only on the first frame the key is pressed (no repeat)
        static bool IsKeyJustPressed(int keycode) { return s_Instance->IsKeyJustPressedImpl(keycode); }
        // Returns true on the frame the key is released
        static bool IsKeyJustReleased(int keycode) { return s_Instance->IsKeyJustReleasedImpl(keycode); }

        static bool IsMouseButtonPressed(int button) { return s_Instance->IsMouseButtonPressedImpl(button); }
        static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
        static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
        static float GetMouseY() { return s_Instance->GetMouseYImpl(); }
        // Returns raw (dx, dy) mouse movement since last frame. Zero on first frame.
        // When cursor is locked, uses GLFW_RAW_MOUSE_MOTION (no OS acceleration).
        static std::pair<float, float> GetMouseDelta() { return s_Instance->GetMouseDeltaImpl(); }
        // Returns (scrollX, scrollY) mouse wheel delta this frame.
        static std::pair<float, float> GetMouseScrollDelta() { return s_Instance->GetMouseScrollDeltaImpl(); }

        // Lock/hide the cursor for FPS-style mouse capture.
        // Enabling also activates raw mouse motion if the platform supports it.
        static void SetCursorLocked(bool locked) { s_Instance->SetCursorLockedImpl(locked); }
        static bool IsCursorLocked() { return s_Instance->IsCursorLockedImpl(); }

        // Called by the platform window scroll callback to feed scroll events.
        static void FeedScroll(float x, float y) { s_Instance->FeedScrollImpl(x, y); }

        // Called by the platform window key/mouse callbacks to feed just-pressed/released events.
        // These accumulate between frames and are snapshotted in Update(), fixing the polling-order bug.
        static void FeedKeyPress(int key)         { s_Instance->FeedKeyPressImpl(key); }
        static void FeedKeyRelease(int key)        { s_Instance->FeedKeyReleaseImpl(key); }
        static void FeedMousePress(int button)     { s_Instance->FeedMousePressImpl(button); }
        static void FeedMouseRelease(int button)   { s_Instance->FeedMouseReleaseImpl(button); }

    protected:
        virtual void UpdateImpl() = 0;
        virtual bool IsKeyPressedImpl(int keycode) = 0;
        virtual bool IsKeyJustPressedImpl(int keycode) = 0;
        virtual bool IsKeyJustReleasedImpl(int keycode) = 0;
        virtual bool IsMouseButtonPressedImpl(int button) = 0;
        virtual std::pair<float, float> GetMousePositionImpl() = 0;
        virtual float GetMouseXImpl() = 0;
        virtual float GetMouseYImpl() = 0;
        virtual std::pair<float, float> GetMouseDeltaImpl() = 0;
        virtual std::pair<float, float> GetMouseScrollDeltaImpl() { return { 0.0f, 0.0f }; }
        virtual void SetCursorLockedImpl(bool locked) {}
        virtual bool IsCursorLockedImpl() const { return false; }
        virtual void FeedScrollImpl(float x, float y) {}
        virtual void FeedKeyPressImpl(int key) {}
        virtual void FeedKeyReleaseImpl(int key) {}
        virtual void FeedMousePressImpl(int button) {}
        virtual void FeedMouseReleaseImpl(int button) {}

    private:
        static Scope<Input> s_Instance;
    };
}
