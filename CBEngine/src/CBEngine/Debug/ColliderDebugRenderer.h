#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Camera/Camera.h"
#include "CBEngine/Components/ColliderComponent.h"

#include <Voxelizer.h>

namespace CB
{
	class ColliderDebugRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		/// Draw the wireframe collider shape for the given collider at the given world transform.
		static void DrawCollider(const ColliderComponent& collider, const Mat4& worldTransform,
			const Camera& camera, const Vector3& color = Vector3(0.0f, 1.0f, 0.0f));

		/// Draw wireframe for each merged box in a voxel compound collider.
		static void DrawVoxelCompound(const voxelizer::VoxelGrid& grid, const Mat4& worldTransform,
			const Camera& camera, const Vector3& color = Vector3(1.0f, 0.5f, 0.0f));

	private:
		static void DrawWireBox(const Vector3& halfExtents, const Vector3& offset,
			const Mat4& worldTransform, const Camera& camera, const Vector3& color);
		static void DrawWireSphere(float radius, const Vector3& offset,
			const Mat4& worldTransform, const Camera& camera, const Vector3& color);
		static void DrawWireCapsule(float radius, float halfHeight, const Vector3& offset,
			const Mat4& worldTransform, const Camera& camera, const Vector3& color);

		/// Upload line vertices and draw them
		static void DrawLines(const std::vector<Vector3>& vertices,
			const Mat4& transform, const Camera& camera, const Vector3& color);

		static void GenerateCircleVertices(std::vector<Vector3>& out, const Vector3& center,
			const Vector3& axis1, const Vector3& axis2, float radius, int segments = 32);

		static Ref<Shader> s_LineShader;
		static uint32_t s_VAO;
		static uint32_t s_VBO;
		static bool s_Initialized;
	};
}
