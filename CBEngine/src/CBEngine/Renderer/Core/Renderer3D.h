#pragma once
#include "CBEngine/Renderer/Camera/Camera.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Renderer/Resources/VertexArray.h"
#include "CBEngine/Renderer/Resources/Buffer.h"

namespace CB
{
	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera,const Vector3& cameraPosition);
		static void EndScene();

		// PBR submit with Material
		static void Submit(const Ref<Shader>& shader,const Ref<Material>& material,const Ref<Mesh>& mesh,
		                   const Mat4& transform = Mat4(1.0f),int entityID = -1,bool useVertexColor = false,
		                   const Ref<Texture2D>& paletteColorTex = nullptr,
		                   const Ref<Texture2D>& paletteMaterialTex = nullptr);

		// Instanced submit for batched voxel entities sharing the same mesh+palette
		static void SubmitVoxelBatch(const Ref<Shader>& shader,const Ref<Material>& material,
		                             const Ref<Mesh>& mesh,
		                             const Ref<Texture2D>& paletteColorTex,
		                             const Ref<Texture2D>& paletteMaterialTex,
		                             const std::vector<Mat4>& transforms,
		                             const std::vector<int>& entityIDs);

		// Raw VAO/IBO submit (used for rope and other procedural geometry)
		static void SubmitVAO(const Ref<Shader>& shader,const Ref<Material>& material,
		                      const Ref<VertexArray>& vao,const Ref<IndexBuffer>& ibo,
		                      const Mat4& transform = Mat4(1.0f),int entityID = -1);

		// Light configuration
		static void SetDirectionalLight(const Vector3& direction,const Vector3& color,float intensity);
		static void SetAmbientLight(const Vector3& color);
	private:
		struct SceneData
		{
			Mat4 ViewProjectionMatrix;
			Vector3 CameraPosition = {0.0f, 0.0f, 0.0f};

			// Directional light
			Vector3 LightDirection = {-0.5f, -1.0f, -0.3f};
			Vector3 LightColor = {1.0f, 1.0f, 1.0f};
			float LightIntensity = 1.0f;
			Vector3 AmbientColor = {0.03f, 0.03f, 0.03f};
		};

		static Scope<SceneData> s_SceneData;
	};
}