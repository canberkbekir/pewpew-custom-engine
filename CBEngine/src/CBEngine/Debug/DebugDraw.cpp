#include "cbpch.h"
#include "DebugDraw.h"
#include "ColliderDebugRenderer.h"

namespace CB
{
	std::vector<DebugLine> DebugDraw::s_Lines;

	void DebugDraw::DrawLine(const Vector3& from,const Vector3& to,
	                         const Vector3& color,float duration) { s_Lines.push_back({from, to, color, duration}); }

	void DebugDraw::DrawRay(const Vector3& origin,const Vector3& direction,
	                        float maxDistance,const Vector3& color,float duration)
	{
		Vector3 end = origin + glm::normalize(direction) * maxDistance;
		s_Lines.push_back({origin, end, color, duration});
	}

	void DebugDraw::Flush(const Camera& camera)
	{
		if (s_Lines.empty())
			return;

		Mat4 identity(1.0f);

		// Batch lines by color for fewer draw calls
		// Simple approach: group consecutive same-color lines
		size_t i = 0;
		while (i < s_Lines.size()) {
			Vector3 currentColor = s_Lines[i].Color;
			std::vector<Vector3> vertices;

			// Collect all lines with the same color starting from index i
			while (i < s_Lines.size() && s_Lines[i].Color == currentColor) {
				vertices.push_back(s_Lines[i].From);
				vertices.push_back(s_Lines[i].To);
				i++;
			}

			ColliderDebugRenderer::DrawLines(vertices, identity, camera, currentColor, true);
		}
	}

	void DebugDraw::Tick(float dt)
	{
		// Remove expired lines (duration <= 0 means one-frame, already rendered)
		for (auto it = s_Lines.begin(); it != s_Lines.end();) {
			it->RemainingTime -= dt;
			if (it->RemainingTime <= 0.0f)
				it = s_Lines.erase(it);
			else
				++it;
		}
	}

	void DebugDraw::Clear() { s_Lines.clear(); }
}