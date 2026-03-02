#pragma once
#include "CBEngine/Math/CoreMath.h"

namespace CB
{
	struct FrustumPlane
	{
		Vector3 Normal{0.0f};
		float D = 0.0f;

		float DistanceToPoint(const Vector3& p) const
		{
			return glm::dot(Normal, p) + D;
		}
	};

	struct Frustum
	{
		FrustumPlane Planes[6]; // Left, Right, Bottom, Top, Near, Far

		static Frustum ExtractFromVP(const Mat4& vp)
		{
			Frustum f;
			// Left
			f.Planes[0].Normal.x = vp[0][3] + vp[0][0];
			f.Planes[0].Normal.y = vp[1][3] + vp[1][0];
			f.Planes[0].Normal.z = vp[2][3] + vp[2][0];
			f.Planes[0].D        = vp[3][3] + vp[3][0];

			// Right
			f.Planes[1].Normal.x = vp[0][3] - vp[0][0];
			f.Planes[1].Normal.y = vp[1][3] - vp[1][0];
			f.Planes[1].Normal.z = vp[2][3] - vp[2][0];
			f.Planes[1].D        = vp[3][3] - vp[3][0];

			// Bottom
			f.Planes[2].Normal.x = vp[0][3] + vp[0][1];
			f.Planes[2].Normal.y = vp[1][3] + vp[1][1];
			f.Planes[2].Normal.z = vp[2][3] + vp[2][1];
			f.Planes[2].D        = vp[3][3] + vp[3][1];

			// Top
			f.Planes[3].Normal.x = vp[0][3] - vp[0][1];
			f.Planes[3].Normal.y = vp[1][3] - vp[1][1];
			f.Planes[3].Normal.z = vp[2][3] - vp[2][1];
			f.Planes[3].D        = vp[3][3] - vp[3][1];

			// Near
			f.Planes[4].Normal.x = vp[0][3] + vp[0][2];
			f.Planes[4].Normal.y = vp[1][3] + vp[1][2];
			f.Planes[4].Normal.z = vp[2][3] + vp[2][2];
			f.Planes[4].D        = vp[3][3] + vp[3][2];

			// Far
			f.Planes[5].Normal.x = vp[0][3] - vp[0][2];
			f.Planes[5].Normal.y = vp[1][3] - vp[1][2];
			f.Planes[5].Normal.z = vp[2][3] - vp[2][2];
			f.Planes[5].D        = vp[3][3] - vp[3][2];

			// Normalize all planes
			for (auto& plane : f.Planes)
			{
				float len = glm::length(plane.Normal);
				if (len > 1e-6f)
				{
					plane.Normal /= len;
					plane.D /= len;
				}
			}

			return f;
		}

		bool TestAABB(const Vector3& min, const Vector3& max) const
		{
			for (const auto& plane : Planes)
			{
				// Find the p-vertex (most positive corner relative to plane normal)
				Vector3 pVertex(
					plane.Normal.x >= 0.0f ? max.x : min.x,
					plane.Normal.y >= 0.0f ? max.y : min.y,
					plane.Normal.z >= 0.0f ? max.z : min.z
				);

				if (plane.DistanceToPoint(pVertex) < 0.0f)
					return false; // Entirely outside this plane
			}
			return true;
		}
	};

	inline void TransformAABB(const Mat4& transform, const Vector3& localMin, const Vector3& localMax,
	                           Vector3& outMin, Vector3& outMax)
	{
		// Transform all 8 corners and find new min/max
		Vector3 corners[8] = {
			{localMin.x, localMin.y, localMin.z},
			{localMax.x, localMin.y, localMin.z},
			{localMin.x, localMax.y, localMin.z},
			{localMax.x, localMax.y, localMin.z},
			{localMin.x, localMin.y, localMax.z},
			{localMax.x, localMin.y, localMax.z},
			{localMin.x, localMax.y, localMax.z},
			{localMax.x, localMax.y, localMax.z},
		};

		outMin = Vector3(std::numeric_limits<float>::max());
		outMax = Vector3(std::numeric_limits<float>::lowest());

		for (const auto& corner : corners)
		{
			Vector3 world = Vector3(transform * Vector4(corner, 1.0f));
			outMin = glm::min(outMin, world);
			outMax = glm::max(outMax, world);
		}
	}
}
