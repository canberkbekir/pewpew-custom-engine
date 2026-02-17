#include "cbpch.h"
#include "ColliderDebugRenderer.h"

#include "CBEngine/Physics/VoxelCollisionShapeGenerator.h"

#include <glad/glad.h>
#include <glm/gtc/constants.hpp>

namespace CB
{
	Ref<Shader> ColliderDebugRenderer::s_LineShader = nullptr;
	uint32_t ColliderDebugRenderer::s_VAO = 0;
	uint32_t ColliderDebugRenderer::s_VBO = 0;
	bool ColliderDebugRenderer::s_Initialized = false;

	static constexpr size_t MAX_LINE_VERTICES = 65536;

	void ColliderDebugRenderer::Init()
	{
		if (s_Initialized)
			return;

		s_LineShader = Shader::Create("assets/shaders/DebugLine.glsl");

		glGenVertexArrays(1, &s_VAO);
		glGenBuffers(1, &s_VBO);

		glBindVertexArray(s_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
		glBufferData(GL_ARRAY_BUFFER, MAX_LINE_VERTICES * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);

		// position attribute (location = 0)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);

		glBindVertexArray(0);

		s_Initialized = true;
	}

	void ColliderDebugRenderer::Shutdown()
	{
		if (s_VBO) { glDeleteBuffers(1, &s_VBO); s_VBO = 0; }
		if (s_VAO) { glDeleteVertexArrays(1, &s_VAO); s_VAO = 0; }
		s_LineShader = nullptr;
		s_Initialized = false;
	}

	void ColliderDebugRenderer::DrawCollider(const ColliderComponent& collider,
		const Mat4& worldTransform, const Camera& camera, const Vector3& color)
	{
		if (!s_Initialized)
			Init();

		switch (collider.Shape)
		{
		case ColliderShape::Box:
			DrawWireBox(collider.HalfExtents, collider.Offset, worldTransform, camera, color);
			break;
		case ColliderShape::Sphere:
			DrawWireSphere(collider.Radius, collider.Offset, worldTransform, camera, color);
			break;
		case ColliderShape::Capsule:
			DrawWireCapsule(collider.CapsuleRadius, collider.CapsuleHalfHeight, collider.Offset,
				worldTransform, camera, color);
			break;
		case ColliderShape::VoxelCompound:
			DrawWireBox(collider.HalfExtents, collider.Offset, worldTransform, camera,
				Vector3(1.0f, 0.5f, 0.0f));
			break;
		}
	}

	void ColliderDebugRenderer::DrawWireBox(const Vector3& halfExtents, const Vector3& offset,
		const Mat4& worldTransform, const Camera& camera, const Vector3& color)
	{
		Vector3 h = halfExtents;
		Vector3 o = offset;

		Vector3 corners[8] = {
			o + Vector3(-h.x, -h.y, -h.z),
			o + Vector3( h.x, -h.y, -h.z),
			o + Vector3( h.x,  h.y, -h.z),
			o + Vector3(-h.x,  h.y, -h.z),
			o + Vector3(-h.x, -h.y,  h.z),
			o + Vector3( h.x, -h.y,  h.z),
			o + Vector3( h.x,  h.y,  h.z),
			o + Vector3(-h.x,  h.y,  h.z),
		};

		std::vector<Vector3> lines;
		lines.reserve(24);

		// Bottom
		lines.push_back(corners[0]); lines.push_back(corners[1]);
		lines.push_back(corners[1]); lines.push_back(corners[2]);
		lines.push_back(corners[2]); lines.push_back(corners[3]);
		lines.push_back(corners[3]); lines.push_back(corners[0]);
		// Top
		lines.push_back(corners[4]); lines.push_back(corners[5]);
		lines.push_back(corners[5]); lines.push_back(corners[6]);
		lines.push_back(corners[6]); lines.push_back(corners[7]);
		lines.push_back(corners[7]); lines.push_back(corners[4]);
		// Verticals
		lines.push_back(corners[0]); lines.push_back(corners[4]);
		lines.push_back(corners[1]); lines.push_back(corners[5]);
		lines.push_back(corners[2]); lines.push_back(corners[6]);
		lines.push_back(corners[3]); lines.push_back(corners[7]);

		DrawLines(lines, worldTransform, camera, color);
	}

	void ColliderDebugRenderer::DrawWireSphere(float radius, const Vector3& offset,
		const Mat4& worldTransform, const Camera& camera, const Vector3& color)
	{
		std::vector<Vector3> lines;
		constexpr int segments = 32;
		lines.reserve(segments * 2 * 3);

		GenerateCircleVertices(lines, offset, Vector3(1, 0, 0), Vector3(0, 1, 0), radius, segments);
		GenerateCircleVertices(lines, offset, Vector3(1, 0, 0), Vector3(0, 0, 1), radius, segments);
		GenerateCircleVertices(lines, offset, Vector3(0, 1, 0), Vector3(0, 0, 1), radius, segments);

		DrawLines(lines, worldTransform, camera, color);
	}

	void ColliderDebugRenderer::DrawWireCapsule(float radius, float halfHeight, const Vector3& offset,
		const Mat4& worldTransform, const Camera& camera, const Vector3& color)
	{
		std::vector<Vector3> lines;
		constexpr int segments = 32;
		constexpr int halfSegs = segments / 2;

		Vector3 top = offset + Vector3(0, halfHeight, 0);
		Vector3 bot = offset - Vector3(0, halfHeight, 0);

		// Horizontal circles at top and bottom
		GenerateCircleVertices(lines, top, Vector3(1, 0, 0), Vector3(0, 0, 1), radius, segments);
		GenerateCircleVertices(lines, bot, Vector3(1, 0, 0), Vector3(0, 0, 1), radius, segments);

		// Top hemisphere arcs
		for (int i = 0; i < halfSegs; i++)
		{
			float a0 = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(halfSegs);
			float a1 = glm::pi<float>() * static_cast<float>(i + 1) / static_cast<float>(halfSegs);
			lines.push_back(top + Vector3(std::cos(a0) * radius, std::sin(a0) * radius, 0));
			lines.push_back(top + Vector3(std::cos(a1) * radius, std::sin(a1) * radius, 0));
			lines.push_back(top + Vector3(0, std::sin(a0) * radius, std::cos(a0) * radius));
			lines.push_back(top + Vector3(0, std::sin(a1) * radius, std::cos(a1) * radius));
		}

		// Bottom hemisphere arcs
		for (int i = 0; i < halfSegs; i++)
		{
			float a0 = glm::pi<float>() + glm::pi<float>() * static_cast<float>(i) / static_cast<float>(halfSegs);
			float a1 = glm::pi<float>() + glm::pi<float>() * static_cast<float>(i + 1) / static_cast<float>(halfSegs);
			lines.push_back(bot + Vector3(std::cos(a0) * radius, std::sin(a0) * radius, 0));
			lines.push_back(bot + Vector3(std::cos(a1) * radius, std::sin(a1) * radius, 0));
			lines.push_back(bot + Vector3(0, std::sin(a0) * radius, std::cos(a0) * radius));
			lines.push_back(bot + Vector3(0, std::sin(a1) * radius, std::cos(a1) * radius));
		}

		// 4 vertical lines
		lines.push_back(top + Vector3( radius, 0, 0)); lines.push_back(bot + Vector3( radius, 0, 0));
		lines.push_back(top + Vector3(-radius, 0, 0)); lines.push_back(bot + Vector3(-radius, 0, 0));
		lines.push_back(top + Vector3(0, 0,  radius)); lines.push_back(bot + Vector3(0, 0,  radius));
		lines.push_back(top + Vector3(0, 0, -radius)); lines.push_back(bot + Vector3(0, 0, -radius));

		DrawLines(lines, worldTransform, camera, color);
	}

	void ColliderDebugRenderer::DrawVoxelCompound(const voxelizer::VoxelGrid& grid,
		const Mat4& worldTransform, const Camera& camera, const Vector3& color)
	{
		if (!s_Initialized)
			Init();

		auto mergedBoxes = VoxelCollisionShapeGenerator::GreedyMerge(grid);
		if (mergedBoxes.empty())
			return;

		Vector3 gridCenter = Vector3(grid.origin) + Vector3(grid.size) * 0.5f * Vector3(grid.voxelSize);

		for (const auto& box : mergedBoxes)
		{
			Vector3 halfExtents(
				(box.Max.x - box.Min.x + 1) * 0.5f * grid.voxelSize.x,
				(box.Max.y - box.Min.y + 1) * 0.5f * grid.voxelSize.y,
				(box.Max.z - box.Min.z + 1) * 0.5f * grid.voxelSize.z);

			Vector3 center = Vector3(grid.origin) + Vector3(
				(box.Min.x + box.Max.x + 1) * 0.5f * grid.voxelSize.x,
				(box.Min.y + box.Max.y + 1) * 0.5f * grid.voxelSize.y,
				(box.Min.z + box.Max.z + 1) * 0.5f * grid.voxelSize.z);

			Vector3 offset = center - gridCenter;

			DrawWireBox(halfExtents, offset, worldTransform, camera, color);
		}
	}

	void ColliderDebugRenderer::GenerateCircleVertices(std::vector<Vector3>& out,
		const Vector3& center, const Vector3& axis1, const Vector3& axis2, float radius, int segments)
	{
		for (int i = 0; i < segments; i++)
		{
			float a0 = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
			float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(segments);

			out.push_back(center + axis1 * (std::cos(a0) * radius) + axis2 * (std::sin(a0) * radius));
			out.push_back(center + axis1 * (std::cos(a1) * radius) + axis2 * (std::sin(a1) * radius));
		}
	}

	void ColliderDebugRenderer::DrawLines(const std::vector<Vector3>& vertices,
		const Mat4& transform, const Camera& camera, const Vector3& color)
	{
		if (vertices.empty() || !s_Initialized)
			return;

		size_t vertexCount = std::min(vertices.size(), MAX_LINE_VERTICES);

		// Upload vertex data
		glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0,
			static_cast<GLsizeiptr>(vertexCount * sizeof(Vector3)),
			&vertices[0]);

		// Bind shader and set uniforms
		s_LineShader->Bind();
		s_LineShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
		s_LineShader->SetMat4("u_Transform", transform);
		s_LineShader->SetFloat3("u_Color", color);

		// Draw
		glBindVertexArray(s_VAO);
		glLineWidth(2.0f);
		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));
		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(0);

		s_LineShader->Unbind();
	}
}
