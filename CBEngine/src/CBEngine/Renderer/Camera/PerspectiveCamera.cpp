#include "cbpch.h"
#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace CB
{
	PerspectiveCamera::PerspectiveCamera(float fovDegrees,float aspectRatio,float nearClip,float farClip)
	{
		SetProjection(fovDegrees, aspectRatio, nearClip, farClip);
		RecalculateViewMatrix();
	}

	void PerspectiveCamera::SetProjection(float fovDegrees,float aspectRatio,float nearClip,float farClip)
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
		return normalize(front);
	}

	Vector3 PerspectiveCamera::GetRightDirection() const
	{
		return normalize(cross(GetForwardDirection(), Vector3(0.0f, 1.0f, 0.0f)));
	}

	Vector3 PerspectiveCamera::GetUpDirection() const
	{
		return normalize(cross(GetRightDirection(), GetForwardDirection()));
	}

	void PerspectiveCamera::RecalculateViewMatrix()
	{
		Vector3 front = GetForwardDirection();
		auto up = Vector3(0.0f, 1.0f, 0.0f);

		m_ViewMatrix = lookAt(m_Position, m_Position + front, up);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}