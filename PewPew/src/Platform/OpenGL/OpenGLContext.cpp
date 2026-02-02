#include "pewpch.h"
#include "OpenGLContext.h"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace PewPew
{
    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
        PEW_CORE_ASSERT(windowHandle, "Window handle is null!")
    }

    void OpenGLContext::Init()
    {
        glfwMakeContextCurrent(m_WindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        PEW_CORE_ASSERT(status, "Failed to initialize Glad!")

        PEW_CORE_INFO("OpenGL Info:");
        PEW_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
        PEW_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
        PEW_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(m_WindowHandle);
    }
}
