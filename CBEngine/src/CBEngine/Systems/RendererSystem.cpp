#include "cbpch.h"
#include "RendererSystem.h"

#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"

namespace CB
{
	void RendererSystem::OnUpdate(Scene* scene, const Camera& camera, const Vector3& cameraPosition,
		const Ref<Shader>& defaultShader, const Ref<Material>& defaultMaterial)
	{
		CB_PROFILE_FUNCTION();

		if (!scene)
			return;

		Renderer3D::BeginScene(camera, cameraPosition);

		SubmitLights(scene);
		SubmitMeshes(scene, defaultShader, defaultMaterial);
		SubmitVoxels(scene, defaultShader, defaultMaterial);

		Renderer3D::EndScene();
	}

	void RendererSystem::SubmitLights(Scene* scene)
	{
		CB_PROFILE_FUNCTION();

		auto& registry = scene->GetRegistry();

		// Safe defaults each frame (prevents garbage if no lights exist)
		Renderer3D::SetAmbientLight(Vector3(0.03f, 0.03f, 0.03f));
		Renderer3D::SetDirectionalLight(Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f), 0.0f);

		// Pick the first enabled directional light
		auto view = registry.view<TransformComponent, DirectionalLightComponent>();
		for (auto e : view)
		{
			auto& tc = view.get<TransformComponent>(e);
			auto& lc = view.get<DirectionalLightComponent>(e);

			if (!lc.Visible)
				continue;

			const Vector3 dir = ExtractForwardFromWorld(tc.GetTransform());
			Renderer3D::SetDirectionalLight(dir, lc.Color, lc.Intensity);
			break;
		}
	}

	void RendererSystem::SubmitMeshes(Scene* scene, const Ref<Shader>& defaultShader, const Ref<Material>& defaultMaterial)
	{
		CB_PROFILE_FUNCTION();

		auto& registry = scene->GetRegistry();

		auto view = registry.view<TransformComponent, MeshRendererComponent>();
		for (auto e : view)
		{
			auto& tc = view.get<TransformComponent>(e);
			auto& mr = view.get<MeshRendererComponent>(e);

			if (!mr.Visible || !mr.MeshAsset)
				continue;

			Ref<Shader> shader = mr.ShaderAsset ? mr.ShaderAsset : defaultShader;
			Ref<Material> material = mr.MaterialAsset ? mr.MaterialAsset : defaultMaterial;

			if (!shader || !material)
				continue;

			Renderer3D::Submit(shader, material, mr.MeshAsset, tc.GetTransform(), (int)(uint32_t)e);
		}
	}

	void RendererSystem::SubmitVoxels(Scene* scene, const Ref<Shader>& defaultShader, const Ref<Material>& defaultMaterial)
	{
		CB_PROFILE_FUNCTION();

		auto& registry = scene->GetRegistry();

		auto view = registry.view<TransformComponent, VoxelRendererComponent>();
		for (auto e : view)
		{
			auto& tc = view.get<TransformComponent>(e);
			auto& vr = view.get<VoxelRendererComponent>(e);

			if (!vr.Visible || !vr.MeshAsset)
				continue;

			Ref<Shader> shader = vr.ShaderAsset ? vr.ShaderAsset : defaultShader;
			Ref<Material> material = vr.MaterialAsset ? vr.MaterialAsset : defaultMaterial;

			if (!shader || !material)
				continue;

			Renderer3D::Submit(shader, material, vr.MeshAsset, tc.GetTransform(), (int)(uint32_t)e, true);
		}
	}

	Vector3 RendererSystem::ExtractForwardFromWorld(const Mat4& world)
	{
		Vector3 zAxis = Vector3(world[2]);
		if (glm::length2(zAxis) < 1e-8f)
			return Vector3(0.0f, -1.0f, 0.0f);

		return -glm::normalize(zAxis);
	}
}
