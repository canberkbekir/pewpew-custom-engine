#include <PewPew.h>

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Layer3D : public PewPew::Layer
{
public:
	Layer3D()
		: Layer("PBRLayer"), m_CameraController(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f), m_ProfilerPanel()
	{
		PEW_PROFILE_FUNCTION();

		// Initialize VoxelizerAPI
		PewPew::VoxelizerAPI::Init();

		// Load original mesh
		{
			PEW_PROFILE_SCOPE("Layer3D::LoadMesh");
			m_OriginalMesh = PewPew::Mesh::Load("assets/models/Matilda.fbx");
		}

		// Voxelize the mesh with texture colors
		{
			PEW_PROFILE_SCOPE("Layer3D::VoxelizeMesh");
			PewPew::VoxelizeSettings settings;
			settings.GridSize = m_GridSize;
			settings.Solid = true;
			m_VoxelData = PewPew::VoxelizerAPI::VoxelizeFromFileWithColors(
				"assets/models/Matilda.fbx",
				"assets/textures/MatildaTexture.png",
				settings);
		}

		// Load PBR shader
		{
			PEW_PROFILE_SCOPE("Layer3D::LoadShader");
			m_Shader = PewPew::Shader::Create("assets/shaders/PBR.glsl");
		}

		// Create materials
		{
			PEW_PROFILE_SCOPE("Layer3D::CreateMaterial");
			// Original mesh material with texture
			m_OriginalMaterial = PewPew::CreateRef<PewPew::Material>();
			m_OriginalMaterial->SetAlbedoMap(PewPew::Texture2D::Create("assets/textures/MatildaTexture.png"));

			// Voxel material (solid color)
			m_VoxelMaterial = PewPew::VoxelizerAPI::CreateVoxelMaterial(m_VoxelColor);
		}

		// Set default light
		PewPew::Renderer3D::SetDirectionalLight(m_LightDirection, m_LightColor, m_LightIntensity);
		PewPew::Renderer3D::SetAmbientLight(m_AmbientColor);
	}

	void OnUpdate(PewPew::Timestep ts) override
	{
		PEW_PROFILE_FUNCTION();

		// Update
		{
			PEW_PROFILE_SCOPE("Layer3D::CameraUpdate");
			m_CameraController.OnUpdate(ts);
		}
		m_MeshRotation += m_RotationSpeed * ts;

		// Render Prep
		{
			PEW_PROFILE_SCOPE("Layer3D::RenderPrep");
			PewPew::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
			PewPew::RenderCommand::Clear();
		}

		// Render Draw
		{
			PEW_PROFILE_SCOPE("Layer3D::RenderDraw");

			PewPew::Renderer3D::BeginScene(m_CameraController.GetCamera(), m_CameraController.GetCamera().GetPosition());

			// Choose which mesh to render
			auto& meshToRender = m_ShowVoxels ? m_VoxelData.Mesh : m_OriginalMesh;
			auto& materialToUse = m_ShowVoxels ? m_VoxelMaterial : m_OriginalMaterial;

			if (meshToRender)
			{
				PEW_PROFILE_SCOPE("Layer3D::MeshSubmit");

				Mat4 transform = glm::scale(Mat4(1.0f), Vector3(m_MeshScale));
				transform = glm::rotate(transform, glm::radians(m_MeshRotation), Vector3(0.0f, 1.0f, 0.0f));

				PewPew::Renderer3D::Submit(m_Shader, materialToUse, meshToRender, transform);
			}

			PewPew::Renderer3D::EndScene();
		}
	}

	void OnImGuiRender() override
	{
		PEW_PROFILE_FUNCTION();

		m_ProfilerPanel.OnImGuiRender();

		ImGui::Begin("PBR Controls");

		// Voxel toggle
		ImGui::Checkbox("Show Voxels", &m_ShowVoxels);

		if (m_ShowVoxels)
		{
			ImGui::Separator();
			ImGui::Text("Voxel Info");
			ImGui::Text("Voxel Count: %llu", m_VoxelData.VoxelCount);
			ImGui::Text("Grid: (%d, %d, %d)",
				m_VoxelData.Grid.size.x,
				m_VoxelData.Grid.size.y,
				m_VoxelData.Grid.size.z);
			if (m_VoxelData.Mesh)
				ImGui::Text("Triangles: %u", m_VoxelData.Mesh->GetIndexCount() / 3);

			// Grid size slider - revoxelize on change
			if (ImGui::SliderInt("Grid Size", &m_GridSize, 8, 128))
			{
				RevoxelizeMesh();
			}

			if (ImGui::Checkbox("Solid Fill", &m_SolidFill))
			{
				RevoxelizeMesh();
			}

			if (ImGui::Checkbox("Use Texture Colors", &m_UseTextureColors))
			{
				RevoxelizeMesh();
			}

			ImGui::Separator();
			ImGui::Text("Voxel Material");

			if (!m_UseTextureColors)
			{
				if (ImGui::ColorEdit3("Voxel Color", &m_VoxelColor.x))
					m_VoxelMaterial->SetAlbedo(m_VoxelColor);
			}
			else
			{
				ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Using texture colors");
			}

			float roughness = m_VoxelMaterial->GetRoughness();
			if (ImGui::SliderFloat("Roughness", &roughness, 0.04f, 1.0f))
				m_VoxelMaterial->SetRoughness(roughness);

			float metallic = m_VoxelMaterial->GetMetallic();
			if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
				m_VoxelMaterial->SetMetallic(metallic);
		}
		else
		{
			if (m_OriginalMesh)
			{
				ImGui::Separator();
				ImGui::Text("Original Mesh");
				ImGui::Text("Triangles: %u", m_OriginalMesh->GetIndexCount() / 3);

				float roughness = m_OriginalMaterial->GetRoughness();
				float metallic = m_OriginalMaterial->GetMetallic();
				Vector3 albedo = m_OriginalMaterial->GetAlbedo();

				if (ImGui::SliderFloat("Roughness", &roughness, 0.04f, 1.0f))
					m_OriginalMaterial->SetRoughness(roughness);
				if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
					m_OriginalMaterial->SetMetallic(metallic);
				if (ImGui::ColorEdit3("Albedo Tint", &albedo.x))
					m_OriginalMaterial->SetAlbedo(albedo);
			}
		}

		ImGui::Separator();
		ImGui::Text("Transform");
		ImGui::SliderFloat("Scale", &m_MeshScale, 0.01f, 10.0f);
		ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 180.0f);

		ImGui::Separator();
		ImGui::Text("Light Settings");

		bool lightChanged = false;
		lightChanged |= ImGui::SliderFloat3("Light Direction", &m_LightDirection.x, -1.0f, 1.0f);
		lightChanged |= ImGui::ColorEdit3("Light Color", &m_LightColor.x);
		lightChanged |= ImGui::SliderFloat("Light Intensity", &m_LightIntensity, 0.0f, 10.0f);
		lightChanged |= ImGui::ColorEdit3("Ambient Color", &m_AmbientColor.x);

		if (lightChanged)
		{
			PewPew::Renderer3D::SetDirectionalLight(m_LightDirection, m_LightColor, m_LightIntensity);
			PewPew::Renderer3D::SetAmbientLight(m_AmbientColor);
		}

		ImGui::End();
	}

	void OnEvent(PewPew::Event& event) override
	{
		PEW_PROFILE_FUNCTION();
		m_ProfilerPanel.OnEvent(event);
		m_CameraController.OnEvent(event);
	}

private:
	PewPew::PerspectiveCameraController m_CameraController;
	PewPew::ProfilerPanel m_ProfilerPanel;

	// Original mesh
	PewPew::Ref<PewPew::Mesh> m_OriginalMesh;
	PewPew::Ref<PewPew::Material> m_OriginalMaterial;

	// Voxelized mesh
	PewPew::VoxelMeshData m_VoxelData;
	PewPew::Ref<PewPew::Material> m_VoxelMaterial;

	PewPew::Ref<PewPew::Shader> m_Shader;

	// Voxel settings
	bool m_ShowVoxels = true;
	bool m_SolidFill = true;
	bool m_UseTextureColors = true;
	int m_GridSize = 32;
	Vector3 m_VoxelColor = { 0.8f, 0.3f, 0.2f };

	void RevoxelizeMesh()
	{
		PewPew::VoxelizeSettings settings;
		settings.GridSize = m_GridSize;
		settings.Solid = m_SolidFill;

		if (m_UseTextureColors)
		{
			m_VoxelData = PewPew::VoxelizerAPI::VoxelizeFromFileWithColors(
				"assets/models/Matilda.fbx",
				"assets/textures/MatildaTexture.png",
				settings);
		}
		else
		{
			m_VoxelData = PewPew::VoxelizerAPI::VoxelizeFromFile("assets/models/Matilda.fbx", settings);
		}
	}

	// Transform
	float m_MeshRotation = 0.0f;
	float m_RotationSpeed = 45.0f;
	float m_MeshScale = 0.1f;

	// Light properties
	Vector3 m_LightDirection = { -0.5f, -1.0f, -0.3f };
	Vector3 m_LightColor = { 1.0f, 0.98f, 0.95f };
	float m_LightIntensity = 2.0f;
	Vector3 m_AmbientColor = { 0.03f, 0.03f, 0.05f };
};

// ============================================================================
// Sandbox Application
// ============================================================================
class Sandbox : public PewPew::Application
{
public:
	Sandbox()
	{
		PEW_PROFILE_FUNCTION();
		PushLayer(new Layer3D());
	}

	~Sandbox() {}
};

PewPew::Application* PewPew::CreateApplication()
{
	PEW_PROFILE_FUNCTION();
	return new Sandbox();
}
