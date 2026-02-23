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

        // Snapshot current key states into prev.
        // GLFW_KEY_SPACE (32) is the lowest valid key; GLFW_KEY_LAST (348) is the highest.
        // Keys outside this range cause GLFW error 65539 "Invalid key".
        for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++)
        {
            int state = glfwGetKey(window, i);
            m_PrevKeyStates[i] = (state == GLFW_PRESS || state == GLFW_REPEAT);
        }

        // Snapshot mouse button states for unified-key JustPressed/JustReleased
        for (int i = 0; i < 8; i++)
            m_PrevMouseButtonStates[i] = (glfwGetMouseButton(window, i) == GLFW_PRESS);

        // Snapshot current mouse position for delta computation this frame.
        // With GLFW_CURSOR_DISABLED the virtual cursor is unbounded, so current-prev
        // gives the total raw accumulated motion since last frame.
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
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
            auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
            return (glfwGetMouseButton(window, button) == GLFW_PRESS) && !m_PrevMouseButtonStates[button];
        }
        if (keycode < 0 || keycode >= 512)
            return false;
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        bool pressedNow = (glfwGetKey(window, keycode) == GLFW_PRESS);
        return pressedNow && !m_PrevKeyStates[keycode];
    }

    bool WindowsInput::IsKeyJustReleasedImpl(int keycode)
    {
        if (keycode >= CB_MOUSE_BUTTON_KEY_BASE)
        {
            int button = keycode - CB_MOUSE_BUTTON_KEY_BASE;
            if (button < 0 || button >= 8) return false;
            auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
            return (glfwGetMouseButton(window, button) != GLFW_PRESS) && m_PrevMouseButtonStates[button];
        }
        if (keycode < 0 || keycode >= 512)
            return false;
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        int state = glfwGetKey(window, keycode);
        bool pressedNow = (state == GLFW_PRESS || state == GLFW_REPEAT);
        return !pressedNow && m_PrevKeyStates[keycode];
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
        if (m_FirstFrame)
            return {0.0f, 0.0f};

        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float dx = static_cast<float>(xpos) - m_PrevMouseX;
        float dy = static_cast<float>(ypos) - m_PrevMouseY;
        return {dx, dy};
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
    }

    void WindowsInput::FeedScrollImpl(float x, float y)
    {
        m_AccumScrollX += x;
        m_AccumScrollY += y;
    }
}
