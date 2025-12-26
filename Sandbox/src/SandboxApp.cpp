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
    }

    void OnEvent(PewPew::Event& event) override
    {
        if (event.GetEventType() == PewPew::EventType::KeyPressed)
        {
            PewPew::KeyPressedEvent& e = dynamic_cast<PewPew::KeyPressedEvent&>(event);
            if (e.GetKeyCode() == PEW_KEY_TAB)
                PEW_TRACE("Tab key is pressed (event)!");
            PEW_TRACE(e.GetKeyCode());
        }
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
