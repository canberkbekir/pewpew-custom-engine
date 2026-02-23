#include "cbpch.h"
#include "WindowsWindow.h"

#include "glad/glad.h"
#include "CBEngine/Events/ApplicationEvent.h"
#include "CBEngine/Events/MouseEvent.h"
#include "CBEngine/Events/KeyEvent.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Input/Input.h"
#include "stb_image.h"

namespace CB
{
    static bool s_GLFWInitialized = false;
    static uint32_t s_GLFWWindowCount = 0;

    static void GLFWErrorCallBack(int error, const char* description)
    {
        CB_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    Scope<Window> Window::Create(const WindowProps& props)
    {
        return CreateScope<WindowsWindow>(props);
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
        CB_PROFILE_FUNCTION();

        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        CB_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

        if (!s_GLFWInitialized)
        {
            // TODO: glfwTerminate on system shutdown
            int success = glfwInit();
            CB_CORE_ASSERT(success, "Could not intialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallBack);

            s_GLFWInitialized = true;
        }

        m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height), m_Data.Title.c_str(),
                                    nullptr, nullptr);
        ++s_GLFWWindowCount;

        // Set window icon
        int iconWidth, iconHeight, iconChannels;
        unsigned char* iconPixels = stbi_load("resources/icon.png", &iconWidth, &iconHeight, &iconChannels, 4);
        if (iconPixels)
        {
            GLFWimage icon;
            icon.width = iconWidth;
            icon.height = iconHeight;
            icon.pixels = iconPixels;
            glfwSetWindowIcon(m_Window, 1, &icon);
            stbi_image_free(iconPixels);
        }

        m_Context = GraphicsContext::Create(m_Window);
        m_Context->Init();

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
                    Input::FeedKeyPress(key);
                    KeyPressedEvent pressed_event(key, 0);
                    data.EventCallback(pressed_event);
                    break;
                }
            case GLFW_RELEASE:
                {
                    Input::FeedKeyRelease(key);
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
                    CB_CORE_WARN("Unhandled key event type {0}", action);
                    break;
                }
            }
        });

        glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            KeyTypedEvent event(keycode);
            data.EventCallback(event);
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            switch (action)
            {
            case GLFW_PRESS:
                {
                    Input::FeedMousePress(button);
                    MouseButtonPressedEvent event(button);
                    data.EventCallback(event);
                    break;
                }
            case GLFW_RELEASE:
                {
                    Input::FeedMouseRelease(button);
                    MouseButtonReleasedEvent event(button);
                    data.EventCallback(event);
                    break;
                }
            default:
                CB_CORE_WARN("Unhandled mouse event type {0}", action);
                break;
            }
        });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
            data.EventCallback(event);

            // Feed into Input system scroll accumulator so Lua can query it
            Input::FeedScroll(static_cast<float>(xOffset), static_cast<float>(yOffset));
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            MouseMovedEvent event(static_cast<float>(xPos), static_cast<float>(yPos));
            data.EventCallback(event);
        });

        glfwSetDropCallback(m_Window, [](GLFWwindow* window, int count, const char** paths)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            std::vector<std::string> pathList;
            pathList.reserve(count);
            for (int i = 0; i < count; i++)
            {
                pathList.emplace_back(paths[i]);
            }
            FileDropEvent event(pathList);
            data.EventCallback(event);
        });
    }

    void WindowsWindow::Shutdown()
    {
        CB_PROFILE_FUNCTION();
        glfwDestroyWindow(m_Window);
        --s_GLFWWindowCount;

        if (s_GLFWWindowCount == 0)
        {
            CB_CORE_INFO("Terminating GLFW");
            glfwTerminate();
            s_GLFWInitialized = false;
        }
    }

    void WindowsWindow::OnUpdate()
    {
        CB_PROFILE_FUNCTION();

        {
            CB_PROFILE_SCOPE("WindowsWindow::PollEvents");
            glfwPollEvents();
        }
        {
            CB_PROFILE_SCOPE("WindowsWindow::SwapBuffers");
            m_Context->SwapBuffers();
        }
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
