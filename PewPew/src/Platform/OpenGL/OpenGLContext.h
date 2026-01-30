#pragma once
#include "PewPew/Renderer/Core/GraphicsContext.h"

struct GLFWwindow;

namespace PewPew
{
    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(GLFWwindow* windowHandle);

        void Init() override;
        void SwapBuffers() override;

    private:
        GLFWwindow* m_WindowHandle;
    };
}
