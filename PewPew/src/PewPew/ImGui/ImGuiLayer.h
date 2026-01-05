#pragma once
#include "PewPew/Layer.h"
#include "PewPew/Events/ApplicationEvent.h"
#include "PewPew/Events/KeyEvent.h"
#include "PewPew/Events/MouseEvent.h"

namespace PewPew {

    class PEW_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnImGuiRender() override;

        void Begin();
        void End();
        
    private:
        float m_Time = 0.0f;
    };

}
