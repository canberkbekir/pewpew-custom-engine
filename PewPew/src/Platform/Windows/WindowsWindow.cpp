#include "pewpch.h"
#include "WindowsWindow.h"

#include "PewPew/Events/ApplicationEvent.h"
#include "PewPew/Events/MouseEvent.h"
#include "PewPew/Events/KeyEvent.h"

namespace PewPew
{
    static bool s_GLFWInitialized = false;

    static void GLFWErrorCallBack(int error, const char* description)
    {
        PEW_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    Window* Window::Create(const WindowProps& props)
    {
        return new WindowsWindow(props);
    }

    WindowsWindow::WindowsWindow(const WindowProps& props)
    {
        WindowsWindow::Init(props);
    }

    WindowsWindow::~WindowsWindow()
    {
        WindowsWindow::Shutdown();
    }

    void WindowsWindow::Init(const WindowProps& props)
    {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        PEW_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

        if (!s_GLFWInitialized)
        {
            // TODO: glfwTerminate on system shutdown
            int success = glfwInit();
            PEW_CORE_ASSERT(success, "Could not intialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallBack);

            s_GLFWInitialized = true;
        }

        m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height), m_Data.Title.c_str(),
                                    nullptr, nullptr);
        glfwMakeContextCurrent(m_Window);
        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(true);

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            data.Width = width;
            data.Height = height;

            WindowResizeEvent event(width, height);
            data.EventCallback(event);
        });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            WindowCloseEvent event;
            data.EventCallback(event);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            switch (action)
            {
            case GLFW_PRESS:
                {
                    KeyPressedEvent pressed_event(key, 0);
                    data.EventCallback(pressed_event);
                    break;
                }
            case GLFW_RELEASE:
                {
                    KeyReleasedEvent released_event(key);
                    data.EventCallback(released_event);
                    break;
                }
            case GLFW_REPEAT:
                {
                    KeyPressedEvent repeat_event(key, 1);
                    data.EventCallback(repeat_event);
                    break;
                }
            default:
                {
                    PEW_CORE_WARN("Unhandled key event type {0}", action);
                    break;
                }
            }
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            switch (action)
            {
            case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(button);
                    data.EventCallback(event);
                    break;
                }
            case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(button);
                    data.EventCallback(event);
                    break;
                }
            default:
                PEW_CORE_WARN("Unhandled mouse event type {0}", action);
                break;
            }
        });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
            data.EventCallback(event);
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            MouseMovedEvent event(static_cast<float>(xPos), static_cast<float>(yPos));
            data.EventCallback(event);
        });
    }

    void WindowsWindow::Shutdown()
    {
        glfwDestroyWindow(m_Window);
    }

    void WindowsWindow::OnUpdate()
    {
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        if (enabled)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);

        m_Data.VSync = enabled;
    }

    bool WindowsWindow::IsVSync() const
    {
        return m_Data.VSync;
    }
}
