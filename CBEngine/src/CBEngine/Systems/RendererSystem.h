#pragma once
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Renderer/Camera/Camera.h"
#include "CBEngine/Renderer/Core/Frustum.h"
#include "CBEngine/Renderer/Core/Framebuffer.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Scene/Scene.h"

namespace CB
{
	class RendererSystem
	{
	public:
		static void OnUpdate(Scene* scene,const Camera& camera,const Vector3& cameraPosition,
		                     const Ref<Shader>& defaultShader = nullptr,
		                     const Ref<Material>& defaultMaterial = nullptr);

		// Shadow mapping
		static void InitShadowMap();
		static void RenderShadowPass(Scene* scene,const Camera& camera,const Vector3& cameraPos);
		static Mat4 ComputeLightSpaceMatrix(const Camera& camera,const Vector3& cameraPos,const Vector3& lightDir);
	private:
		static void SubmitLights(Scene* scene);
		static void SubmitMeshes(Scene* scene,const Ref<Shader>& defaultShader,const Ref<Material>& defaultMaterial);
		static void SubmitVoxels(Scene* scene,const Ref<Shader>& defaultShader,const Ref<Material>& defaultMaterial);

		static Vector3 ExtractForwardFromWorld(const Mat4& world);

		// Frustum culling
		static Frustum s_Frustum;

		// Shadow mapping state
		static Ref<Framebuffer> s_ShadowFBO;
		static Ref<Shader> s_ShadowShader;
		static bool s_ShadowsInitialized;
	};
}