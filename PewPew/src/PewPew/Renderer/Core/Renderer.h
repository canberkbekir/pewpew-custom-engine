#pragma once
#include "PewPew/Renderer/Camera/Camera.h"
#include "PewPew/Renderer/Core/RendererAPI.h"
#include "PewPew/Renderer/Resources/Shader.h"
#include "PewPew/Renderer/Resources/VertexArray.h"
#include "PewPew/Renderer/Resources/Material.h"
#include "PewPew/Renderer/Resources/Mesh.h"

namespace PewPew {

	class Renderer
	{
	public:
		static void Init();
		static void OnWindowResize(uint32_t width, uint32_t height);
		
		static void BeginScene(const Camera& camera);
		static void BeginScene(const Camera& camera, const Vector3& cameraPosition);
		static void EndScene();

		// Original submit (for backwards compatibility)
		static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const Mat4& transform = Mat4(1.0f));

		// PBR submit with Material
		static void Submit(const Ref<Shader>& shader, const Ref<Material>& material, const Ref<Mesh>& mesh, const Mat4& transform = Mat4(1.0f));

		// Light configuration
		static void SetDirectionalLight(const Vector3& direction, const Vector3& color, float intensity);
		static void SetAmbientLight(const Vector3& color);

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			Mat4 ViewProjectionMatrix;
			Vector3 CameraPosition = { 0.0f, 0.0f, 0.0f };

			// Directional light
			Vector3 LightDirection = { -0.5f, -1.0f, -0.3f };
			Vector3 LightColor = { 1.0f, 1.0f, 1.0f };
			float LightIntensity = 1.0f;
			Vector3 AmbientColor = { 0.03f, 0.03f, 0.03f };
		};

		static SceneData* s_SceneData;
	};


}