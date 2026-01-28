#include "pewpch.h"
#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace PewPew
{
    PerspectiveCamera::PerspectiveCamera(float fovDegrees, float aspectRatio, float nearClip, float farClip)
    {
        SetProjection(fovDegrees, aspectRatio, nearClip, farClip);
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetProjection(float fovDegrees, float aspectRatio, float nearClip, float farClip)
    {
        m_ProjectionMatrix = glm::perspective(glm::radians(fovDegrees), aspectRatio, nearClip, farClip);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    Vector3 PerspectiveCamera::GetForwardDirection() const
    {
        Vector3 front;
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        return glm::normalize(front);
    }

    Vector3 PerspectiveCamera::GetRightDirection() const
    {
        return glm::normalize(glm::cross(GetForwardDirection(), Vector3(0.0f, 1.0f, 0.0f)));
    }

    Vector3 PerspectiveCamera::GetUpDirection() const
    {
        return glm::normalize(glm::cross(GetRightDirection(), GetForwardDirection()));
    }

    void PerspectiveCamera::RecalculateViewMatrix()
    {
        Vector3 front = GetForwardDirection();
        Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

        m_ViewMatrix = glm::lookAt(m_Position, m_Position + front, up);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}
