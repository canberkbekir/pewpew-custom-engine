#pragma once
#include "PewPew/Core/Layer.h"
#include "PewPew/Events/ApplicationEvent.h"
#include "PewPew/Events/KeyEvent.h"
#include "PewPew/Events/MouseEvent.h"

namespace PewPew
{
    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnImGuiRender() override;

        void Begin();
        void End();

    private:
        float m_Time = 0.0f;
    };
}
