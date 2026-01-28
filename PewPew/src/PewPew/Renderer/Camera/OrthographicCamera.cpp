#include "pewpch.h"
#include "OrthographicCamera.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

PewPew::OrthographicCamera::OrthographicCamera( float left,  float right,  float bottom,  float top)
{
    constexpr float NearPlane = -1.0f;
    constexpr float FarPlane  =  1.0f;

    m_ProjectionMatrix = glm::ortho(
        left,
        right,
        bottom,
        top,
        NearPlane,
        FarPlane
    );

    m_ViewMatrix = Mat4(1.0f);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void PewPew::OrthographicCamera::RecalculateViewMatrix()
{
    const Mat4 translation = glm::translate(
        Mat4(1.0f),
        m_Position
    );

    const Mat4 rotation = glm::rotate(
        Mat4(1.0f),
        glm::radians(m_Rotation),
        Vector3(0.0f, 0.0f, 1.0f)
    );

    const Mat4 transform = translation * rotation;

    m_ViewMatrix = glm::inverse(transform);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

