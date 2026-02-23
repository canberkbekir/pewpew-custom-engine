#include "cbpch.h"
#include "WindowsInput.h"

#include "WindowsWindow.h"
#include "CBEngine/Core/Application.h"
#include "CBEngine/Input/KeyCodes.h"

namespace CB
{
    Scope<Input> Input::s_Instance = CreateScope<WindowsInput>();

    void WindowsInput::UpdateImpl()
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        // Snapshot current key/mouse states for IsKeyPressed (held-down queries).
        for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++)
        {
            int state = glfwGetKey(window, i);
            m_PrevKeyStates[i] = (state == GLFW_PRESS || state == GLFW_REPEAT);
        }
        for (int i = 0; i < 8; i++)
            m_PrevMouseButtonStates[i] = (glfwGetMouseButton(window, i) == GLFW_PRESS);

        // Snapshot callback-fed just-pressed/released accumulators into per-frame arrays.
        // The accumulators were filled by FeedKeyPress/Release during the previous frame's
        // glfwPollEvents(). Swap into frame arrays and clear for next frame.
        m_FrameJustPressed       = m_AccumJustPressed;
        m_FrameJustReleased      = m_AccumJustReleased;
        m_FrameMouseJustPressed  = m_AccumMouseJustPressed;
        m_FrameMouseJustReleased = m_AccumMouseJustReleased;
        m_AccumJustPressed.fill(false);
        m_AccumJustReleased.fill(false);
        m_AccumMouseJustPressed.fill(false);
        m_AccumMouseJustReleased.fill(false);

        // Compute and store the frame delta BEFORE overwriting m_Prev.
        // GetMouseDeltaImpl() returns this stored value; querying glfwGetCursorPos live
        // during OnUpdate would always return the same position as m_Prev because
        // glfwPollEvents hasn't run yet (it's called at the end of the frame).
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (!m_FirstFrame)
        {
            m_FrameMouseDeltaX = static_cast<float>(xpos) - m_PrevMouseX;
            m_FrameMouseDeltaY = static_cast<float>(ypos) - m_PrevMouseY;
        }
        else
        {
            m_FrameMouseDeltaX = 0.0f;
            m_FrameMouseDeltaY = 0.0f;
        }
        m_PrevMouseX = static_cast<float>(xpos);
        m_PrevMouseY = static_cast<float>(ypos);

        // Snapshot scroll accumulator into per-frame value and reset
        m_FrameScrollX = m_AccumScrollX;
        m_FrameScrollY = m_AccumScrollY;
        m_AccumScrollX = 0.0f;
        m_AccumScrollY = 0.0f;

        m_FirstFrame = false;
    }

    bool WindowsInput::IsKeyPressedImpl(int keycode)
    {
        // Unified: codes >= CB_MOUSE_BUTTON_KEY_BASE route to mouse buttons
        if (keycode >= CB_MOUSE_BUTTON_KEY_BASE)
        {
            int button = keycode - CB_MOUSE_BUTTON_KEY_BASE;
            if (button > GLFW_MOUSE_BUTTON_LAST) return false;
            auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
            return glfwGetMouseButton(window, button) == GLFW_PRESS;
        }
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetKey(window, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool WindowsInput::IsKeyJustPressedImpl(int keycode)
    {
        if (keycode >= CB_MOUSE_BUTTON_KEY_BASE)
        {
            int button = keycode - CB_MOUSE_BUTTON_KEY_BASE;
            if (button < 0 || button >= 8) return false;
            return m_FrameMouseJustPressed[button];
        }
        if (keycode < 0 || keycode >= 512) return false;
        return m_FrameJustPressed[keycode];
    }

    bool WindowsInput::IsKeyJustReleasedImpl(int keycode)
    {
        if (keycode >= CB_MOUSE_BUTTON_KEY_BASE)
        {
            int button = keycode - CB_MOUSE_BUTTON_KEY_BASE;
            if (button < 0 || button >= 8) return false;
            return m_FrameMouseJustReleased[button];
        }
        if (keycode < 0 || keycode >= 512) return false;
        return m_FrameJustReleased[keycode];
    }

    bool WindowsInput::IsMouseButtonPressedImpl(int button)
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, button);
        return state == GLFW_PRESS;
    }

    std::pair<float, float> WindowsInput::GetMousePositionImpl()
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        return {static_cast<float>(xpos), static_cast<float>(ypos)};
    }

    float WindowsInput::GetMouseXImpl()
    {
        auto [x, y] = GetMousePositionImpl();
        return x;
    }

    float WindowsInput::GetMouseYImpl()
    {
        auto [x, y] = GetMousePositionImpl();
        return y;
    }

    std::pair<float, float> WindowsInput::GetMouseDeltaImpl()
    {
        return {m_FrameMouseDeltaX, m_FrameMouseDeltaY};
    }

    std::pair<float, float> WindowsInput::GetMouseScrollDeltaImpl()
    {
        return { m_FrameScrollX, m_FrameScrollY };
    }

    void WindowsInput::SetCursorLockedImpl(bool locked)
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        if (locked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // Enable raw mouse motion to bypass OS pointer acceleration.
            // This is the key fix for "jittery fast movement" — the OS applies
            // a non-linear curve to cursor speed that feels wrong for FPS look.
            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
        else
        {
            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        m_CursorLocked = locked;

        // Re-sync the position snapshot to prevent a large delta spike on the
        // frame the lock state changes (cursor teleports to/from center).
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        m_PrevMouseX = static_cast<float>(xpos);
        m_PrevMouseY = static_cast<float>(ypos);
        m_FrameMouseDeltaX = 0.0f;
        m_FrameMouseDeltaY = 0.0f;
    }

    void WindowsInput::FeedScrollImpl(float x, float y)
    {
        m_AccumScrollX += x;
        m_AccumScrollY += y;
    }

    void WindowsInput::FeedKeyPressImpl(int key)
    {
        if (key >= 0 && key < 512)
            m_AccumJustPressed[key] = true;
    }

    void WindowsInput::FeedKeyReleaseImpl(int key)
    {
        if (key >= 0 && key < 512)
            m_AccumJustReleased[key] = true;
    }

    void WindowsInput::FeedMousePressImpl(int button)
    {
        if (button >= 0 && button < 8)
            m_AccumMouseJustPressed[button] = true;
    }

    void WindowsInput::FeedMouseReleaseImpl(int button)
    {
        if (button >= 0 && button < 8)
            m_AccumMouseJustReleased[button] = true;
    }
}
