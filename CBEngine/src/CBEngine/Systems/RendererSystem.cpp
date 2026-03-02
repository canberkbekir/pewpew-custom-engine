#include "cbpch.h"
#include "RendererSystem.h"

#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/PointLightComponent.h"
#include "CBEngine/Components/SpotLightComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Voxel/VoxelizerAPI.h"

#include <glad/glad.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <unordered_map>

namespace CB
{
	// Static member definitions
	Frustum RendererSystem::s_Frustum;
	Ref<Framebuffer> RendererSystem::s_ShadowFBO;
	Ref<Shader> RendererSystem::s_ShadowShader;
	bool RendererSystem::s_ShadowsInitialized = false;

	void RendererSystem::OnUpdate(Scene* scene,const Camera& camera,const Vector3& cameraPosition,
	                              const Ref<Shader>& defaultShader,const Ref<Material>& defaultMaterial)
	{
		CB_PROFILE_SCOPE_CAT("RendererSystem::OnUpdate", "Rendering");

		if (!scene)
			return;

		// Extract frustum planes for culling
		s_Frustum = Frustum::ExtractFromVP(camera.GetViewProjectionMatrix());

		// Shadow pass (before main scene)
		RenderShadowPass(scene, camera, cameraPosition);

		Renderer3D::BeginScene(camera, cameraPosition);

		SubmitLights(scene);
		SubmitMeshes(scene, defaultShader, defaultMaterial);
		SubmitVoxels(scene, defaultShader, defaultMaterial);

		Renderer3D::EndScene();
	}

	void RendererSystem::SubmitLights(Scene* scene)
	{
		CB_PROFILE_SCOPE_CAT("RendererSystem::SubmitLights", "Rendering");

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

		// Collect point lights
		{
			std::vector<Vector3> positions, colors;
			std::vector<float> intensities, ranges;

			auto plView = registry.view<TransformComponent, PointLightComponent>();
			for (auto e : plView) {
				auto& tc = plView.get<TransformComponent>(e);
				auto& pl = plView.get<PointLightComponent>(e);
				if (!pl.Visible) continue;
				positions.push_back(Vector3(tc.GetTransform()[3]));
				colors.push_back(pl.Color);
				intensities.push_back(pl.Intensity);
				ranges.push_back(pl.Range);
			}

			if (!positions.empty())
				Renderer3D::SetPointLights(positions.data(), colors.data(), intensities.data(),
				                           ranges.data(), static_cast<int>(positions.size()));
			else
				Renderer3D::SetPointLights(nullptr, nullptr, nullptr, nullptr, 0);
		}

		// Collect spot lights
		{
			std::vector<Vector3> positions, directions, colors;
			std::vector<float> intensities, ranges, innerCos, outerCos;

			auto slView = registry.view<TransformComponent, SpotLightComponent>();
			for (auto e : slView) {
				auto& tc = slView.get<TransformComponent>(e);
				auto& sl = slView.get<SpotLightComponent>(e);
				if (!sl.Visible) continue;
				positions.push_back(Vector3(tc.GetTransform()[3]));
				directions.push_back(ExtractForwardFromWorld(tc.GetTransform()));
				colors.push_back(sl.Color);
				intensities.push_back(sl.Intensity);
				ranges.push_back(sl.Range);
				innerCos.push_back(glm::cos(glm::radians(sl.InnerAngleDegrees)));
				outerCos.push_back(glm::cos(glm::radians(sl.OuterAngleDegrees)));
			}

			if (!positions.empty())
				Renderer3D::SetSpotLights(positions.data(), directions.data(), colors.data(),
				                          intensities.data(), ranges.data(), innerCos.data(),
				                          outerCos.data(), static_cast<int>(positions.size()));
			else
				Renderer3D::SetSpotLights(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
		}
	}

	void RendererSystem::SubmitMeshes(Scene* scene,const Ref<Shader>& defaultShader,
	                                  const Ref<Material>& defaultMaterial)
	{
		CB_PROFILE_SCOPE_CAT("RendererSystem::SubmitMeshes", "Rendering");

		auto& registry = scene->GetRegistry();

		auto view = registry.view<TransformComponent, MeshRendererComponent>();
		for (auto e : view) {
			auto& tc = view.get<TransformComponent>(e);
			auto& mr = view.get<MeshRendererComponent>(e);

			if (!mr.Visible || !mr.MeshAsset)
				continue;

			// Frustum culling
			{
				Vector3 localMin, localMax;
				if (mr.MeshAsset->GetBounds(localMin, localMax))
				{
					Vector3 worldMin, worldMax;
					TransformAABB(tc.GetTransform(), localMin, localMax, worldMin, worldMax);
					if (!s_Frustum.TestAABB(worldMin, worldMax))
						continue;
				}
			}

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
		CB_PROFILE_SCOPE_CAT("RendererSystem::SubmitVoxels", "Rendering");

		auto& registry = scene->GetRegistry();

		Ref<Shader> voxelShader = VoxelizerAPI::GetVoxelShader();
		if (!voxelShader)
			voxelShader = defaultShader;

		// Batch key: entities sharing the same vmesh UUID + same mesh pointer.
		// Destroyed/tinted entities get their own mesh copy, so they must not
		// share a batch with entities still using the original mesh.
		struct BatchKey
		{
			uint64_t VoxelMeshUUID;
			const Mesh* MeshPtr; // distinguish original vs cloned/rebuilt meshes
			bool operator==(const BatchKey& other) const {
				return VoxelMeshUUID == other.VoxelMeshUUID && MeshPtr == other.MeshPtr;
			}
		};

		struct BatchKeyHash
		{
			size_t operator()(const BatchKey& key) const {
				size_t h = std::hash<uint64_t>()(key.VoxelMeshUUID);
				h ^= std::hash<const void*>()(key.MeshPtr) + 0x9e3779b9 + (h << 6) + (h >> 2);
				return h;
			}
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

			// Frustum culling
			{
				Vector3 localMin, localMax;
				if (vr.MeshAsset->GetBounds(localMin, localMax))
				{
					Vector3 worldMin, worldMax;
					TransformAABB(tc.GetTransform(), localMin, localMax, worldMin, worldMax);
					if (!s_Frustum.TestAABB(worldMin, worldMax))
						continue;
				}
			}

			// Only batch palette-based voxel entities
			if (vr.HasPalette && vr.PaletteColorTexture && vr.PaletteMaterialTexture && vr.VoxelMeshUUID.IsValid()) {
				BatchKey key{vr.VoxelMeshUUID, vr.MeshAsset.get()};
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

	// ---- Shadow Mapping ----

	void RendererSystem::InitShadowMap()
	{
		if (s_ShadowsInitialized)
			return;

		// Create depth-only FBO for shadow map (2048x2048)
		FramebufferSpecification spec;
		spec.Width = 2048;
		spec.Height = 2048;
		spec.Attachments = {{FramebufferTextureFormat::DEPTH_COMPONENT32F}};
		s_ShadowFBO = Framebuffer::Create(spec);

		s_ShadowShader = Shader::Create("assets/shaders/ShadowDepth.glsl");

		s_ShadowsInitialized = true;
	}

	Mat4 RendererSystem::ComputeLightSpaceMatrix(const Camera& camera,const Vector3& cameraPos,
	                                                const Vector3& lightDir)
	{
		// Get inverse VP to reconstruct frustum corners
		Mat4 invVP = glm::inverse(camera.GetViewProjectionMatrix());

		// 8 NDC corners
		Vector3 ndcCorners[8] = {
			{-1, -1, -1}, { 1, -1, -1}, {-1,  1, -1}, { 1,  1, -1},
			{-1, -1,  1}, { 1, -1,  1}, {-1,  1,  1}, { 1,  1,  1}
		};

		Vector3 frustumCorners[8];
		Vector3 center(0.0f);
		for (int i = 0; i < 8; i++)
		{
			Vector4 world = invVP * Vector4(ndcCorners[i], 1.0f);
			frustumCorners[i] = Vector3(world) / world.w;
			center += frustumCorners[i];
		}
		center /= 8.0f;

		// Light view matrix
		Vector3 lightDirN = glm::normalize(lightDir);
		Mat4 lightView = glm::lookAt(center - lightDirN * 50.0f, center, Vector3(0.0f, 1.0f, 0.0f));

		// If light is nearly parallel to up, use alternative up vector
		if (glm::abs(glm::dot(lightDirN, Vector3(0.0f, 1.0f, 0.0f))) > 0.99f)
			lightView = glm::lookAt(center - lightDirN * 50.0f, center, Vector3(0.0f, 0.0f, 1.0f));

		// Find bounding box in light-view space
		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();

		for (const auto& corner : frustumCorners)
		{
			Vector4 lv = lightView * Vector4(corner, 1.0f);
			minX = std::min(minX, lv.x);
			maxX = std::max(maxX, lv.x);
			minY = std::min(minY, lv.y);
			maxY = std::max(maxY, lv.y);
			minZ = std::min(minZ, lv.z);
			maxZ = std::max(maxZ, lv.z);
		}

		// Extend Z range to capture shadow casters behind camera
		float zMult = 10.0f;
		if (minZ < 0.0f) minZ *= zMult;
		else minZ /= zMult;
		if (maxZ < 0.0f) maxZ /= zMult;
		else maxZ *= zMult;

		Mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
		return lightProj * lightView;
	}

	void RendererSystem::RenderShadowPass(Scene* scene,const Camera& camera,const Vector3& cameraPos)
	{
		CB_PROFILE_SCOPE_CAT("RendererSystem::RenderShadowPass", "Rendering");

		if (!scene)
		{
			Renderer3D::SetShadowsEnabled(false);
			return;
		}

		// Find directional light with CastShadows
		auto& registry = scene->GetRegistry();
		Vector3 lightDir(0.0f, -1.0f, 0.0f);
		bool foundShadowLight = false;

		auto view = registry.view<TransformComponent, DirectionalLightComponent>();
		for (auto e : view) {
			auto& tc = view.get<TransformComponent>(e);
			auto& lc = view.get<DirectionalLightComponent>(e);

			if (!lc.Visible || !lc.CastShadows)
				continue;

			lightDir = ExtractForwardFromWorld(tc.GetTransform());
			foundShadowLight = true;
			break;
		}

		if (!foundShadowLight)
		{
			Renderer3D::SetShadowsEnabled(false);
			return;
		}

		// Lazy init
		InitShadowMap();

		if (!s_ShadowFBO || !s_ShadowShader)
		{
			Renderer3D::SetShadowsEnabled(false);
			return;
		}

		Mat4 lightSpaceMatrix = ComputeLightSpaceMatrix(camera, cameraPos, lightDir);

		// Save current FBO and viewport state so we can restore after shadow pass
		GLint previousFBO = 0;
		GLint previousViewport[4] = {};
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
		glGetIntegerv(GL_VIEWPORT, previousViewport);

		// Bind shadow FBO and render depth
		s_ShadowFBO->Bind();
		glViewport(0, 0, 2048, 2048);
		glClear(GL_DEPTH_BUFFER_BIT);

		s_ShadowShader->Bind();
		s_ShadowShader->SetMat4("u_LightSpaceVP", lightSpaceMatrix);
		s_ShadowShader->SetInt("u_UseInstancing", 0);

		// Render mesh entities
		{
			auto meshView = registry.view<TransformComponent, MeshRendererComponent>();
			for (auto e : meshView)
			{
				auto& tc = meshView.get<TransformComponent>(e);
				auto& mr = meshView.get<MeshRendererComponent>(e);
				if (!mr.Visible || !mr.MeshAsset) continue;

				s_ShadowShader->SetMat4("u_Transform", tc.GetTransform());
				mr.MeshAsset->Bind();
				RenderCommand::DrawIndexed(mr.MeshAsset->GetVertexArray());
			}
		}

		// Render voxel entities
		{
			auto voxelView = registry.view<TransformComponent, VoxelRendererComponent>();
			for (auto e : voxelView)
			{
				auto& tc = voxelView.get<TransformComponent>(e);
				auto& vr = voxelView.get<VoxelRendererComponent>(e);
				if (!vr.Visible || !vr.MeshAsset) continue;

				s_ShadowShader->SetMat4("u_Transform", tc.GetTransform());
				vr.MeshAsset->Bind();
				RenderCommand::DrawIndexed(vr.MeshAsset->GetVertexArray());
			}
		}

		// Restore previous FBO and viewport
		glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
		glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

		// Pass shadow data to renderer
		Renderer3D::SetShadowData(lightSpaceMatrix, s_ShadowFBO->GetDepthAttachmentRendererID());
		Renderer3D::SetShadowsEnabled(true);
	}

	Vector3 RendererSystem::ExtractForwardFromWorld(const Mat4& world)
	{
		auto zAxis = Vector3(world[2]);
		if (length2(zAxis) < 1e-8f)
			return Vector3(0.0f, -1.0f, 0.0f);

		return -normalize(zAxis);
	}

}
