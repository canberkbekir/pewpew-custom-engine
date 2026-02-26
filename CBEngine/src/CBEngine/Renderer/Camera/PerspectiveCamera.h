#pragma once
#include "Camera.h"

namespace CB
{
	class PerspectiveCamera : public Camera
	{
	public:
		PerspectiveCamera(float fovDegrees,float aspectRatio,float nearClip,float farClip);

		void SetProjection(float fovDegrees,float aspectRatio,float nearClip,float farClip);

		const Vector3& GetPosition() const { return m_Position; }

		void SetPosition(const Vector3& position)
		{
			m_Position = position;
			RecalculateViewMatrix();
		}

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

		void SetRotation(float pitch,float yaw)
		{
			m_Pitch = pitch;
			m_Yaw = yaw;
			RecalculateViewMatrix();
		}

		Vector3 GetForwardDirection() const;
		Vector3 GetRightDirection() const;
		Vector3 GetUpDirection() const;

		const Mat4& GetProjectionMatrix() const override { return m_ProjectionMatrix; }
		const Mat4& GetViewMatrix() const override { return m_ViewMatrix; }
		const Mat4& GetViewProjectionMatrix() const override { return m_ViewProjectionMatrix; }
	private:
		void RecalculateViewMatrix();

		Mat4 m_ProjectionMatrix;
		Mat4 m_ViewMatrix;
		Mat4 m_ViewProjectionMatrix;

		Vector3 m_Position = {0.0f, 0.0f, 0.0f};
		float m_Pitch = 0.0f; // Up/down rotation in degrees
		float m_Yaw = -90.0f; // Left/right rotation in degrees (start looking at -Z)
	};
}