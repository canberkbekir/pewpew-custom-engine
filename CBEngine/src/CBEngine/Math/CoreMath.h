#pragma once
#include <glm/glm.hpp>

namespace CB
{
	namespace Math = glm;
}

namespace PMath = CB::Math;

using Vector2 = PMath::vec2;
using Vector3 = PMath::vec3;
using Vector4 = PMath::vec4;

using Mat3 = PMath::mat3;
using Mat4 = PMath::mat4;

using Quaternion = PMath::quat;