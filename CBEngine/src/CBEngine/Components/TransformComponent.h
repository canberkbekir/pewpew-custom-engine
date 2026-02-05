#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/quaternion.hpp"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Core/UUID.h"
#include <vector>

namespace CB {

    struct TransformComponent
    { 
        Vector3 Position = Vector3(0, 0, 0);
        Vector3 Rotation = Vector3(0, 0, 0);
        Vector3 Scale = Vector3(1, 1, 1);

        // Hierarchy
        UUID Parent{ 0 };
        std::vector<UUID> Children;

        // Cached world transform
        Mat4 WorldMatrix{ 1.0f };
        bool Dirty = true;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const Vector3& position) : Position(position) {}
 
        Mat4 GetLocalTransform() const
        {
            Mat4 rotation = glm::toMat4(glm::quat(Rotation));
            return glm::translate(Mat4(1.0f), Position) * rotation * glm::scale(Mat4(1.0f), Scale);
        }
 
        Mat4 GetTransform() const
        {
            if (Dirty)
                return GetLocalTransform();
            return WorldMatrix;
        }

        // World space getters
        Vector3 GetWorldPosition() const
        {
            if (Dirty && !HasParent())
                return Position;
            return Vector3(WorldMatrix[3]);
        }
        
        Vector3 GetForward() const { return glm::normalize(Vector3(WorldMatrix[2])); }
        Vector3 GetRight() const { return glm::normalize(Vector3(WorldMatrix[0])); }
        Vector3 GetUp() const { return glm::normalize(Vector3(WorldMatrix[1])); }
        
        bool HasParent() const { return Parent != 0; }
        bool HasChildren() const { return !Children.empty(); } 
    };

}