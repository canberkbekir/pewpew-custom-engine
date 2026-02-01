#include "pewpch.h"
#include "PerspectiveCameraController.h"

#include "PewPew/Input/Input.h"
#include "PewPew/Input/KeyCodes.h"
#include "PewPew/Input/MouseButtonCodes.h"

namespace PewPew
{
    PerspectiveCameraController::PerspectiveCameraController(float fovDegrees, float aspectRatio, float nearClip,
                                                             float farClip, bool enableRotation)
        : m_Camera(fovDegrees, aspectRatio, nearClip, farClip)
          , m_AspectRatio(aspectRatio)
          , m_FOV(fovDegrees)
          , m_NearClip(nearClip)
          , m_FarClip(farClip)
          , m_RotationEnabled(enableRotation)
    {
    }

    void PerspectiveCameraController::OnUpdate(Timestep ts)
    {
        if (!m_Enabled)
            return;

        const float deltaTime = ts.GetSeconds();

        Vector3 position = m_Camera.GetPosition();

        if (Input::IsKeyPressed(PEW_KEY_W))
            position += m_Camera.GetForwardDirection() * (m_MoveSpeed * deltaTime);
        if (Input::IsKeyPressed(PEW_KEY_S))
            position -= m_Camera.GetForwardDirection() * (m_MoveSpeed * deltaTime);

        if (Input::IsKeyPressed(PEW_KEY_D))
            position += m_Camera.GetRightDirection() * (m_MoveSpeed * deltaTime);
        if (Input::IsKeyPressed(PEW_KEY_A))
            position -= m_Camera.GetRightDirection() * (m_MoveSpeed * deltaTime);

        if (Input::IsKeyPressed(PEW_KEY_E))
            position += m_Camera.GetUpDirection() * (m_MoveSpeed * deltaTime);
        if (Input::IsKeyPressed(PEW_KEY_Q))
            position -= m_Camera.GetUpDirection() * (m_MoveSpeed * deltaTime);

        m_Camera.SetPosition(position);


        if (m_RotationEnabled && Input::IsMouseButtonPressed(PEW_MOUSE_BUTTON_2))
        {
            auto [mx, my] = Input::GetMousePosition();
            const float x = mx;
            const float y = my;

            if (m_FirstMouse)
            {
                m_LastMouseX = x;
                m_LastMouseY = y;
                m_FirstMouse = false;
            }

            float xOffset = x - m_LastMouseX;
            float yOffset = m_LastMouseY - y;

            m_LastMouseX = x;
            m_LastMouseY = y;

            xOffset *= m_MouseSensitivity;
            yOffset *= m_MouseSensitivity;

            float pitch = m_Camera.GetPitch();
            float yaw = m_Camera.GetYaw();

            pitch += yOffset;
            yaw += xOffset;

            // Clamp pitch to avoid flipping
            pitch = std::clamp(pitch, -89.0f, 89.0f);

            m_Camera.SetRotation(pitch, yaw);
        }
        else
        {
            m_FirstMouse = true;
        }
    }

    void PerspectiveCameraController::OnEvent(Event& e)
    {
        if (!m_Enabled)
            return;

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(PEW_BIND_EVENT_FN(PerspectiveCameraController::OnMouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(PEW_BIND_EVENT_FN(PerspectiveCameraController::OnWindowResized));
    }

    void PerspectiveCameraController::SetViewportSize(float width, float height)
    {
        if (height <= 0.0f)
            return;

        m_AspectRatio = width / height;
        m_Camera.SetProjection(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
    }

    bool PerspectiveCameraController::OnMouseScrolled(MouseScrolledEvent& e)
    {
        m_FOV -= e.GetYOffset() * 2.0f;

        // Typical usable FOV range
        m_FOV = std::clamp(m_FOV, 30.0f, 90.0f);

        m_Camera.SetProjection(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
        return false;
    }

    bool PerspectiveCameraController::OnWindowResized(WindowResizeEvent& e)
    {
        const float w = static_cast<float>(e.GetWidth());
        const float h = static_cast<float>(e.GetHeight());
        SetViewportSize(w, h);
        return false;
    }
}
