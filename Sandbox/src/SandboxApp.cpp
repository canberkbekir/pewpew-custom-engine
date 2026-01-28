#include <PewPew.h>

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Layer3D : public PewPew::Layer
{
public:
	Layer3D()
		: Layer("PBRLayer"), m_CameraController(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f)
	{
		// Load mesh (now includes tangent/bitangent for normal mapping)
		m_Mesh = PewPew::Mesh::Load("assets/models/Hand.fbx");

		// Load PBR shader
		m_Shader = PewPew::Shader::Create("assets/shaders/PBR.glsl");

		// Create and configure material
		m_Material = PewPew::Material::Create();
		m_Material->SetAlbedoMap(PewPew::Texture2D::Create("assets/textures/HAND_C.jpg"));
		m_Material->SetNormalMap(PewPew::Texture2D::Create("assets/textures/HAND_N.jpg"));
		// Note: Using HAND_S.jpg as roughness map (green channel will be used)
		m_Material->SetRoughnessMap(PewPew::Texture2D::Create("assets/textures/HAND_S.jpg"));
		m_Material->SetRoughness(0.5f);
		m_Material->SetMetallic(0.0f);

		// Set default light
		PewPew::Renderer::SetDirectionalLight(
			m_LightDirection,
			m_LightColor,
			m_LightIntensity
		);
		PewPew::Renderer::SetAmbientLight(m_AmbientColor);
	}

	void OnUpdate(PewPew::Timestep ts) override
	{
		m_CameraController.OnUpdate(ts);
		m_MeshRotation += m_RotationSpeed * ts;

		// Render
		PewPew::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
		PewPew::RenderCommand::Clear();

		// Begin scene with camera position for specular calculations
		PewPew::Renderer::BeginScene(m_CameraController.GetCamera(), m_CameraController.GetCamera().GetPosition());

		if (m_Mesh)
		{
			// Apply scale and rotation to mesh
			Mat4 transform = glm::scale(Mat4(1.0f), Vector3(m_MeshScale));
			transform = glm::rotate(transform, glm::radians(m_MeshRotation), Vector3(0.0f, 1.0f, 0.0f));

			// Submit with PBR material
			PewPew::Renderer::Submit(m_Shader, m_Material, m_Mesh, transform);
		}

		PewPew::Renderer::EndScene();
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("PBR Controls");

		if (m_Mesh)
		{
			ImGui::Text("Mesh: %u indices", m_Mesh->GetIndexCount());
			ImGui::SliderFloat("Scale", &m_MeshScale, 0.01f, 10.0f);
			ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 180.0f);

			ImGui::Separator();
			ImGui::Text("Shader Mode");

			// Shader mode dropdown
			const char* shaderModes[] = { "Normal (PBR)", "Stylized/Toon", "Voxel", "Pixel" };
			int shaderMode = m_Material->GetShaderMode();
			if (ImGui::Combo("Shader", &shaderMode, shaderModes, IM_ARRAYSIZE(shaderModes)))
				m_Material->SetShaderMode(shaderMode);

			// Mode-specific controls
			if (shaderMode == 0) // Normal PBR
			{
				float smoothAmount = m_Material->GetSmoothAmount();
				if (ImGui::SliderFloat("Smooth Shading", &smoothAmount, 0.0f, 1.0f, "%.2f"))
					m_Material->SetSmoothAmount(smoothAmount);

				float stylizedAmount = m_Material->GetStylizedAmount();
				if (ImGui::SliderFloat("Stylized Blend", &stylizedAmount, 0.0f, 1.0f, "%.2f"))
					m_Material->SetStylizedAmount(stylizedAmount);
			}
			else if (shaderMode == 2) // Voxel
			{
				float voxelSize = m_Material->GetVoxelSize();
				if (ImGui::SliderFloat("Voxel Size", &voxelSize, 0.01f, 1.0f, "%.3f"))
					m_Material->SetVoxelSize(voxelSize);
			}
			else if (shaderMode == 3) // Pixel
			{
				float pixelSize = m_Material->GetPixelSize();
				if (ImGui::SliderFloat("Pixel Resolution", &pixelSize, 8.0f, 256.0f, "%.0f"))
					m_Material->SetPixelSize(pixelSize);
			}

			ImGui::Separator();
			ImGui::Text("Material Properties");

			// Material scalar properties (used when no texture is bound)
			float roughness = m_Material->GetRoughness();
			float metallic = m_Material->GetMetallic();
			Vector3 albedo = m_Material->GetAlbedo();

			if (ImGui::SliderFloat("Roughness", &roughness, 0.04f, 1.0f))
				m_Material->SetRoughness(roughness);
			if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
				m_Material->SetMetallic(metallic);
			if (ImGui::ColorEdit3("Albedo Tint", &albedo.x))
				m_Material->SetAlbedo(albedo);

			ImGui::Separator();
			ImGui::Text("Light Settings");

			bool lightChanged = false;
			lightChanged |= ImGui::SliderFloat3("Light Direction", &m_LightDirection.x, -1.0f, 1.0f);
			lightChanged |= ImGui::ColorEdit3("Light Color", &m_LightColor.x);
			lightChanged |= ImGui::SliderFloat("Light Intensity", &m_LightIntensity, 0.0f, 10.0f);
			lightChanged |= ImGui::ColorEdit3("Ambient Color", &m_AmbientColor.x);

			if (lightChanged)
			{
				PewPew::Renderer::SetDirectionalLight(m_LightDirection, m_LightColor, m_LightIntensity);
				PewPew::Renderer::SetAmbientLight(m_AmbientColor);
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Mesh not loaded!");
		}

		ImGui::End();
	}

	void OnEvent(PewPew::Event& event) override
	{
		m_CameraController.OnEvent(event);
	}

private:
	PewPew::PerspectiveCameraController m_CameraController;

	PewPew::Ref<PewPew::Mesh> m_Mesh;
	PewPew::Ref<PewPew::Shader> m_Shader;
	PewPew::Ref<PewPew::Material> m_Material;

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
		PushLayer(new Layer3D());
	}

	~Sandbox() {}
};

PewPew::Application* PewPew::CreateApplication()
{
	return new Sandbox();
}
