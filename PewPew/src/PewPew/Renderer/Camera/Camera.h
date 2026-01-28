#pragma once
#include "PewPew/Math/CoreMath.h"

namespace PewPew
{
    class Camera
    {
    public:
        virtual ~Camera() = default;

        virtual const Mat4& GetProjectionMatrix() const = 0;
        virtual const Mat4& GetViewMatrix() const = 0;
        virtual const Mat4& GetViewProjectionMatrix() const = 0;
    };
}
