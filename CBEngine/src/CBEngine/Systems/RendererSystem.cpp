#include "cbpch.h"
#include "RendererSystem.h"

#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Utils/VoxelizerAPI.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <unordered_map>

namespace CB
{
	void RendererSystem::OnUpdate(Scene* scene,const Camera& camera,const Vector3& cameraPosition,
	                              const Ref<Shader>& defaultShader,const Ref<Material>& defaultMaterial)
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
		for (auto e : view) {
			auto& tc = view.get<TransformComponent>(e);
			auto& lc = view.get<DirectionalLightComponent>(e);

			if (!lc.Visible)
				continue;

			const Vector3 dir = ExtractForwardFromWorld(tc.GetTransform());
			Renderer3D::SetDirectionalLight(dir, lc.Color, lc.Intensity);
			break;
		}
	}

	void RendererSystem::SubmitMeshes(Scene* scene,const Ref<Shader>& defaultShader,
	                                  const Ref<Material>& defaultMaterial)
	{
		CB_PROFILE_FUNCTION();

		auto& registry = scene->GetRegistry();

		auto view = registry.view<TransformComponent, MeshRendererComponent>();
		for (auto e : view) {
			auto& tc = view.get<TransformComponent>(e);
			auto& mr = view.get<MeshRendererComponent>(e);

			if (!mr.Visible || !mr.MeshAsset)
				continue;

			Ref<Shader> shader = mr.ShaderAsset ? mr.ShaderAsset : defaultShader;
			Ref<Material> material = mr.MaterialAsset ? mr.MaterialAsset : defaultMaterial;

			if (!shader || !material)
				continue;

			Renderer3D::Submit(shader, material, mr.MeshAsset, tc.GetTransform(),
			                   static_cast<int>(static_cast<uint32_t>(e)));
		}
	}

	void RendererSystem::SubmitVoxels(Scene* scene,const Ref<Shader>& defaultShader,
	                                  const Ref<Material>& defaultMaterial)
	{
		CB_PROFILE_FUNCTION();

		auto& registry = scene->GetRegistry();

		Ref<Shader> voxelShader = VoxelizerAPI::GetVoxelShader();
		if (!voxelShader)
			voxelShader = defaultShader;

		// Batch key: entities sharing the same vmesh UUID + palette textures
		struct BatchKey
		{
			uint64_t VoxelMeshUUID;
			bool operator==(const BatchKey& other) const { return VoxelMeshUUID == other.VoxelMeshUUID; }
		};

		struct BatchKeyHash
		{
			size_t operator()(const BatchKey& key) const { return std::hash<uint64_t>()(key.VoxelMeshUUID); }
		};

		struct BatchEntry
		{
			Ref<Mesh> MeshAsset;
			Ref<Texture2D> PaletteColorTexture;
			Ref<Texture2D> PaletteMaterialTexture;
			std::vector<Mat4> Transforms;
			std::vector<int> EntityIDs;
		};

		std::unordered_map<BatchKey, BatchEntry, BatchKeyHash> batches;
		std::vector<entt::entity> nonBatchable; // Non-palette voxels

		auto view = registry.view<TransformComponent, VoxelRendererComponent>();
		for (auto e : view) {
			auto& tc = view.get<TransformComponent>(e);
			auto& vr = view.get<VoxelRendererComponent>(e);

			if (!vr.Visible || !vr.MeshAsset)
				continue;

			// Only batch palette-based voxel entities
			if (vr.HasPalette && vr.PaletteColorTexture && vr.PaletteMaterialTexture && vr.VoxelMeshUUID.IsValid()) {
				BatchKey key{(vr.VoxelMeshUUID)};
				auto& batch = batches[key];
				if (!batch.MeshAsset) {
					batch.MeshAsset = vr.MeshAsset;
					batch.PaletteColorTexture = vr.PaletteColorTexture;
					batch.PaletteMaterialTexture = vr.PaletteMaterialTexture;
				}
				batch.Transforms.push_back(tc.GetTransform());
				batch.EntityIDs.push_back(static_cast<int>(static_cast<uint32_t>(e)));
			}
			else { nonBatchable.push_back(e); }
		}

		// Submit batched groups
		for (auto& [key, batch] : batches) {
			if (batch.Transforms.size() > 1) {
				Renderer3D::SubmitVoxelBatch(voxelShader, defaultMaterial,
				                             batch.MeshAsset,
				                             batch.PaletteColorTexture,
				                             batch.PaletteMaterialTexture,
				                             batch.Transforms,
				                             batch.EntityIDs);
			}
			else {
				Renderer3D::Submit(voxelShader, defaultMaterial, batch.MeshAsset,
				                   batch.Transforms[0], batch.EntityIDs[0], false,
				                   batch.PaletteColorTexture, batch.PaletteMaterialTexture);
			}
		}

		// Submit non-batchable entities individually
		for (auto e : nonBatchable) {
			auto& tc = view.get<TransformComponent>(e);
			auto& vr = view.get<VoxelRendererComponent>(e);
			Renderer3D::Submit(voxelShader, defaultMaterial, vr.MeshAsset, tc.GetTransform(),
			                   static_cast<int>(static_cast<uint32_t>(e)),
			                   !vr.HasPalette, vr.PaletteColorTexture, vr.PaletteMaterialTexture);
		}
	}

	Vector3 RendererSystem::ExtractForwardFromWorld(const Mat4& world)
	{
		auto zAxis = Vector3(world[2]);
		if (length2(zAxis) < 1e-8f)
			return Vector3(0.0f, -1.0f, 0.0f);

		return -normalize(zAxis);
	}

}