#include <PewPew.h>

#include "imgui/imgui.h"

class ExampleLayer : public PewPew::Layer
{
public:
    ExampleLayer()
        : Layer("Example")
    {
    }

    void OnUpdate() override
    {
        if (PewPew::Input::IsKeyPressed(PEW_KEY_TAB))
            PEW_TRACE("Tab key is pressed (poll)!");
    }

    virtual void OnImGuiRender() override
    {
        ImGui::Begin("Test");
        ImGui::Text("Hello World");
        ImGui::End();
    }

    void OnEvent(PewPew::Event& event) override
    {
        if (event.GetEventType() == PewPew::EventType::KeyPressed)
        {
            auto& e = dynamic_cast<PewPew::KeyPressedEvent&>(event);
            if (e.GetKeyCode() == PEW_KEY_TAB)
                PEW_TRACE("Tab key is pressed (event)!");
            PEW_TRACE("{0}", static_cast<char>(e.GetKeyCode()));
        }
    }

};

class Sandbox : public PewPew::Application
{
public:
    Sandbox()
    {
        PushLayer(new ExampleLayer());
    }

    ~Sandbox()
    {

    }

};

PewPew::Application* PewPew::CreateApplication()
{
    return new Sandbox();
}