#pragma once

#include "CameraController.h"
#include "PerspectiveCamera.h"

#include "PewPew/Core/Timestep.h"
#include "PewPew/Events/MouseEvent.h"
#include "PewPew/Events/ApplicationEvent.h"

namespace PewPew
{
    class PerspectiveCameraController : public CameraController
    {
    public:
        PerspectiveCameraController(
            float fovDegrees,
            float aspectRatio,
            float nearClip,
            float farClip,
            bool enableRotation = true
        );

        void OnUpdate(Timestep ts) override;
        void OnEvent(Event& e) override;

        PerspectiveCamera& GetCamera() override { return m_Camera; }
        const PerspectiveCamera& GetCamera() const override { return m_Camera; }

        void SetViewportSize(float width, float height) override;

        // Camera settings accessors
        float GetMoveSpeed() const { return m_MoveSpeed; }
        void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

        float GetMouseSensitivity() const { return m_MouseSensitivity; }
        void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }

        float GetFOV() const { return m_FOV; }
        void SetFOV(float fov);

        float GetNearClip() const { return m_NearClip; }
        void SetNearClip(float nearClip);

        float GetFarClip() const { return m_FarClip; }
        void SetFarClip(float farClip);

    private:
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

        PerspectiveCamera m_Camera;

        float m_AspectRatio;
        float m_FOV;
        float m_NearClip;
        float m_FarClip;

        bool m_RotationEnabled = true;

        // Movement
        float m_MoveSpeed = 6.0f;
        float m_MouseSensitivity = 0.1f;

        // Mouse state
        float m_LastMouseX = 0.0f;
        float m_LastMouseY = 0.0f;
        bool m_FirstMouse = true;
    };
}
