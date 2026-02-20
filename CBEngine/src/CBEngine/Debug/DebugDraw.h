#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Camera/Camera.h"

#include <vector>

namespace CB
{
	struct DebugLine
	{
		Vector3 From;
		Vector3 To;
		Vector3 Color;
		float RemainingTime;
	};

	class DebugDraw
	{
	public:
		static void DrawLine(const Vector3& from, const Vector3& to,
			const Vector3& color = Vector3(0.0f, 1.0f, 0.0f), float duration = 0.0f);

		static void DrawRay(const Vector3& origin, const Vector3& direction,
			float maxDistance = 100.0f, const Vector3& color = Vector3(1.0f, 0.0f, 0.0f),
			float duration = 0.0f);

		/// Render all buffered lines. Call once per frame from the viewport.
		static void Flush(const Camera& camera);

		/// Remove expired lines. Call once per frame with delta time.
		static void Tick(float dt);

		/// Clear all lines immediately.
		static void Clear();

	private:
		static std::vector<DebugLine> s_Lines;
	};
}
