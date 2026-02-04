#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/quaternion.hpp"
#include "PewPew/Math/CoreMath.h" 

namespace PewPew {

    struct TransformComponent
    {
        Vector3 Position = Vector3(0, 0, 0);
        Vector3 Rotation = Vector3(0, 0, 0);
        Vector3 Scale = Vector3(1, 1, 1);

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const Vector3& position) : Position(position) {}

        Mat4 GetTransform() const
        {
            Mat4 rotation = glm::toMat4(glm::quat(Rotation));

            return glm::translate(Mat4(1.0f), Position) * rotation * glm::scale(Mat4(1.0f), Scale);
        }
    };

}