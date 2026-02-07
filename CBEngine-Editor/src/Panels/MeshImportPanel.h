#pragma once

#include "Panel.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Core/Framebuffer.h"
#include "CBEngine/Renderer/Camera/PerspectiveCameraController.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Utils/VoxelizationTask.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"

#include <filesystem>
#include <chrono>

namespace CB
{
	class MeshImportPanel : public Panel
	{
	public:
		MeshImportPanel();

		void OnUpdate(Timestep ts);
		void OnImGuiRender() override;
		void OnEvent(Event& e) override;

		// Open for a new file. outputDirectory: where .mesh/.vmesh go (empty = source file's parent).
		void OpenForFile(const std::filesystem::path& filePath, const std::filesystem::path& outputDirectory = {});

		// Open for reimport of an existing processed asset
		void OpenForReimport(UUID assetUUID);

		bool IsOpen() const { return m_IsOpen; }

	private:
		void RenderRawViewport();
		void RenderVoxelViewport();
		void RenderSettings();
		void RenderMaterialSlots();
		void RenderTextureSlots();
		void RenderOutputSection();

		void RegenVoxelPreview();
		void GenerateVoxelMeshAsset(String outputName,std::filesystem::path sourceDir,std::filesystem::path relativeSourcePath,
		                   UUID sourceMeshUUID);
		void GenerateProccessedMeshAsset(String outputName,std::filesystem::path sourceDir,
		                                 std::filesystem::path relativeSourcePath,UUID sourceMeshUUID);
		void ConfirmImport();
		void Close();

		// Auto-fit camera to mesh bounds
		void FitCameraToMesh();

	private:
		bool m_IsOpen = false;
		std::filesystem::path m_SourceFilePath;
		std::filesystem::path m_OutputDirectory; // Where .mesh/.vmesh are written
		String m_OutputName;

		// Framebuffers
		Ref<Framebuffer> m_RawFramebuffer;
		Ref<Framebuffer> m_VoxelFramebuffer;

		// Cameras
		PerspectiveCameraController m_RawCameraController;
		float m_AutoRotationAngle = 0.0f;

		// Rendering
		Ref<Shader> m_PreviewShader;
		Ref<Material> m_PreviewMaterial;

		// Meshes
		Ref<Mesh> m_RawMesh;
		Ref<Mesh> m_VoxelMesh;

		// Async voxelization
		VoxelizationTask m_VoxelTask;
		VoxelizeSettings m_VoxelSettings;
		bool m_GenerateVoxelMesh = false;

		// Preview palette textures
		Ref<Texture2D> m_PreviewPaletteColorTex;
		Ref<Texture2D> m_PreviewPaletteMaterialTex;
		bool m_PreviewHasPalette = false;

		// Color texture for voxelization
		UUID m_ColorTextureUUID;
		String m_ColorTexturePath;

		// Slots
		std::vector<MaterialSlot> m_MaterialSlots;
		std::vector<TextureSlot> m_TextureSlots;

		// Reimport state
		bool m_IsReimport = false;
		UUID m_ReimportUUID;

		// Viewport state
		Vector2 m_RawViewportSize = { 400.0f, 300.0f };
		Vector2 m_VoxelViewportSize = { 400.0f, 300.0f };
		bool m_RawViewportHovered = false;

		// Debounce for voxel regeneration
		bool m_VoxelSettingsChanged = false;
		std::chrono::steady_clock::time_point m_LastSettingsChangeTime;
		static constexpr float s_DebounceDelayMs = 300.0f;

		// Output name buffer
		char m_OutputNameBuffer[256] = "";
	};
}
