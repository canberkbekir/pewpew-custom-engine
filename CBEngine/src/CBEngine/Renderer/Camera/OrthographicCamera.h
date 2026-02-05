#pragma once
#include "Camera.h"

namespace CB
{
    class OrthographicCamera : public Camera
    {
    public:
        OrthographicCamera(float left, float right, float bottom, float top);

        const Vector3& GetPosition() const { return m_Position; }

        void SetPosition(const Vector3& position)
        {
            m_Position = position;
            RecalculateViewMatrix();
        }

        float GetRotation() const { return m_Rotation; }

        void SetRotation(float rotation)
        {
            m_Rotation = rotation;
            RecalculateViewMatrix();
        }

        const Mat4& GetProjectionMatrix() const override { return m_ProjectionMatrix; }
        const Mat4& GetViewMatrix() const override { return m_ViewMatrix; }
        const Mat4& GetViewProjectionMatrix() const override { return m_ViewProjectionMatrix; }

    private:
        void RecalculateViewMatrix();
        Mat4 m_ProjectionMatrix;
        Mat4 m_ViewMatrix;
        Mat4 m_ViewProjectionMatrix;

        Vector3 m_Position = {0.0f, 0.0f, 0.0f};
        float m_Rotation = 0.0f;
    };
}
