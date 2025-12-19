#include <PewPew.h>

class ExampleLayer : public PewPew::Layer
{
public:
    ExampleLayer()
          : Layer("Example")
    {
    }

    void OnUpdate() override
    {
        PEW_CORE_INFO("ExampleLayer::Update");
    }

    void OnEvent(PewPew::Event& event) override
    {
        PEW_CORE_TRACE(event.ToString());
    }
    
};

class Sandbox : public PewPew::Application
{
public:
    Sandbox()
    {
        PushLayer(new ExampleLayer());
        PushOverlay(new PewPew::ImGuiLayer());
    }

    ~Sandbox() override
    {
    }
};

PewPew::Application* PewPew::CreateApplication()
{
    return new Sandbox();
}
