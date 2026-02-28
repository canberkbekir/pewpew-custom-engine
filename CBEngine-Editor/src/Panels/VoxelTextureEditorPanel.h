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
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Utils/VoxelMaterialType.h"
#include "CBEngine/Voxel/Substance/SubstanceID.h"

#include <set>

namespace CB
{
	class VoxelTextureEditorPanel : public Panel
	{
	public:
		VoxelTextureEditorPanel();

		void OnUpdate(Timestep ts);
		void OnImGuiRender() override;
		void OnEvent(Event& e) override;

		// Open for a specific vtex asset
		void OpenForAsset(UUID vtexUUID);

		bool IsOpen() const { return m_IsOpen; }
	private:
		void RenderModelPreview();
		void RenderMaterialMapView();
		void RenderPaletteEditor();
		void RenderPBROverrides();

		void RebuildMaterialMapMesh();
		void FitCameraToMesh();

		// Build a clipped mesh showing only voxels at or below m_SliceY
		void RebuildSlicedMesh();

		// 2D grid view for layer painting
		void RebuildSliceLookup();
		void Render2DGridView();
		void HandleGridPainting();

		// Grouped palette
		void RebuildColorGroups();

		bool m_IsOpen = false;
		UUID m_VtexUUID;
		Ref<VoxelTextureAsset> m_Vtex;

		// Source vmesh data
		UUID m_VmeshUUID;
		Ref<VoxelMeshAsset> m_Vmesh;

		// Framebuffers
		Ref<Framebuffer> m_ModelFramebuffer;
		Ref<Framebuffer> m_MaterialMapFramebuffer;

		// Camera (shared between both viewports)
		PerspectiveCameraController m_CameraController;
		float m_AutoRotationAngle = 0.0f;

		// Rendering
		Ref<Shader> m_PreviewShader;
		Ref<Material> m_PreviewMaterial;

		// Model preview mesh (real colors)
		Ref<Mesh> m_ModelMesh;
		Ref<Texture2D> m_PaletteColorTexture;
		Ref<Texture2D> m_PaletteMaterialTexture;
		bool m_HasPalette = false;

		// Material map mesh (colored by material type)
		Ref<Mesh> m_MaterialMapMesh;
		Ref<Texture2D> m_MaterialMapPaletteTexture;

		// Sliced material map mesh (for layer view)
		Ref<Mesh> m_SlicedMesh;
		Ref<Texture2D> m_SlicedPaletteTexture;

		// Viewport state
		Vector2 m_ModelViewportSize = {400.0f, 300.0f};
		Vector2 m_MaterialMapViewportSize = {400.0f, 300.0f};
		bool m_ModelViewportHovered = false;
		bool m_MaterialMapViewportHovered = false;

		// Layer view state (Dwarf Fortress style)
		bool m_ShowSliceView = false;
		int m_SliceY = 0;
		int m_PrevSliceY = -1;

		// Brush (selected palette entry for painting)
		int m_SelectedBrushIndex = -1;

		// 2D grid view state
		Vector2 m_GridPanOffset = {0.0f, 0.0f};
		float m_GridZoom = 1.0f;
		std::vector<int32_t> m_SliceLookup; // (x * sizeZ + z) -> filled voxel index, -1 = empty
		bool m_SliceLookupDirty = true;
		glm::ivec2 m_HoveredCell = {-1, -1};
		bool m_IsPainting = false;

		// Grouped palette editor
		struct PaletteColorGroup
		{
			Vector3 AverageColor;
			SubstanceID MaterialSubstance = "Stone";
			std::vector<uint8_t> EntryIndices;
			bool Expanded = false;
		};

		std::vector<PaletteColorGroup> m_ColorGroups;
		bool m_ColorGroupsDirty = true;
		bool m_ShowGroupedView = true;

		// Dirty flag for material map rebuild
		bool m_MaterialMapDirty = false;

		// Deferred rebuild: true while user is actively painting (mouse held down)
		bool m_PaintingActive = false;

		// When only palette PBR properties change, re-upload textures without rebuilding geometry
		bool m_PaletteTextureDirty = false;

		// Texture picker for "Generate from Texture"
		UUID m_SourceTextureUUID = UUID(0);

		// Material picker for "Generate from Material"
		UUID m_SourceMaterialUUID = UUID(0);

		// Add palette entry state
		float m_NewPaletteColor[3] = {0.5f, 0.5f, 0.5f};
		int m_NewPaletteMaterialType = 0;

		// Custom brush tracking (parallel to m_Vtex->CustomBrushes)
		std::vector<uint8_t> m_CustomBrushPaletteIndices;
		bool IsCustomBrush(uint8_t paletteIndex) const;
		void RemoveBrush(uint8_t paletteIndex);

		// Tracks palette indices hidden via the [X] button (original or custom)
		std::set<uint8_t> m_RemovedPaletteIndices;
	};
}