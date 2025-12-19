#include <PewPew.h>

class Sandbox : public PewPew::Application
{
public:
    Sandbox()
    {
    }

    ~Sandbox() override
    {
    }
};

PewPew::Application* PewPew::CreateApplication()
{
    return new Sandbox();
}
