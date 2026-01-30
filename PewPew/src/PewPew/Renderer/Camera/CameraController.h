#pragma once
#include "pewpch.h"

#include "Camera.h"
#include "PewPew/Core/TimeStep.h"
#include "PewPew/Events/Event.h"

namespace PewPew
{
    class CameraController
    {
    public:
        virtual ~CameraController() = default;

        virtual void OnUpdate(Timestep ts) = 0;
        virtual void OnEvent(Event& e) = 0;

        virtual Camera& GetCamera() = 0;
        virtual const Camera& GetCamera() const = 0;

        virtual void SetViewportSize(float width, float height) = 0;
        virtual void SetEnabled(bool enabled) { m_Enabled = enabled; }
        bool IsEnabled() const { return m_Enabled; }

    protected:
        bool m_Enabled = true;
    };
}
