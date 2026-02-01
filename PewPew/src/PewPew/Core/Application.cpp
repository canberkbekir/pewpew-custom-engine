#include "pewpch.h"
#include "Application.h"

#include "Log.h"
#include "PewPew/Debug/Instrumentor.h"

#include "PewPew/Input/Input.h"
#include "TimeStep.h"
#include "GLFW/glfw3.h"
#include "PewPew/Renderer/Core/Renderer.h"
#include "PewPew/Renderer/Core/Renderer3D.h"

namespace PewPew
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        PEW_PROFILE_FUNCTION();

        PEW_CORE_ASSERT(!s_Instance, "Application already exists!")
        s_Instance = this;

        m_Window = Window::Create();
        m_Window->SetEventCallback(PEW_BIND_EVENT_FN(OnEvent));

        Renderer::Init();
        Renderer3D::Init();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        PEW_PROFILE_FUNCTION();
        Renderer::Shutdown();
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Layer* layer)
    {
        m_LayerStack.PushOverlay(layer);
    }

    void Application::OnEvent(Event& e)
    {
        PEW_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(PEW_BIND_EVENT_FN(OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(PEW_BIND_EVENT_FN(OnWindowResize));

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
                break;
        }
    }

    void Application::Run()
    {
        while (m_Running)
        {
            PEW_PROFILE_BEGIN_FRAME();
            PEW_PROFILE_SCOPE("RunLoop");

            float time = static_cast<float>(glfwGetTime());
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            if (!m_Minimized)
            {
                {
                    PEW_PROFILE_SCOPE("LayerStack OnUpdate");
                    for (Layer* layer : m_LayerStack)
                        layer->OnUpdate(timestep);
                }
            }

            {
                PEW_PROFILE_SCOPE("LayerStack OnImGuiRender");
                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerStack)
                    layer->OnImGuiRender();
                m_ImGuiLayer->End();
            }

            m_Window->OnUpdate();
            PEW_PROFILE_END_FRAME();
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.GetHeight() == 0 || e.GetWidth() == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
        return false;
    }
}
