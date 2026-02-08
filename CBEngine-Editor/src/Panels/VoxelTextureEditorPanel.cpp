#include "VoxelTextureEditorPanel.h"

#include <imgui.h>

#include "CBEngine/Core/Log.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "../Widgets/AssetPicker.h"

#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <cmath>

namespace CB
{
	VoxelTextureEditorPanel::VoxelTextureEditorPanel()
		: Panel("Voxel Texture Editor", false)
		, m_CameraController(45.0f, 4.0f / 3.0f, 0.1f, 100.0f)
	{
		FramebufferSpecification fbSpec;
		fbSpec.Width = 400;
		fbSpec.Height = 300;
		m_ModelFramebuffer = Framebuffer::Create(fbSpec);
		m_MaterialMapFramebuffer = Framebuffer::Create(fbSpec);

		m_PreviewShader = Shader::Create("assets/shaders/PBR.glsl");
		m_PreviewMaterial = CreateRef<Material>();
		m_PreviewMaterial->SetAlbedo({ 0.8f, 0.8f, 0.8f });
		m_PreviewMaterial->SetRoughness(0.5f);
		m_PreviewMaterial->SetMetallic(0.0f);
	}

	void VoxelTextureEditorPanel::FitCameraToMesh()
	{
		if (!m_ModelMesh || !m_ModelMesh->HasCPUData())
		{
			// Fallback: estimate from grid dimensions
			if (m_Vmesh)
			{
				const auto& grid = m_Vmesh->GridData;
				Vector3 extents = Vector3(grid.size) * grid.voxelSize;
				float maxExtent = glm::max(extents.x, glm::max(extents.y, extents.z));
				if (maxExtent < 0.0001f) maxExtent = 1.0f;

				Vector3 center = grid.origin + extents * 0.5f;
				float fovRad = glm::radians(m_CameraController.GetFOV());
				float distance = (maxExtent * 0.5f) / std::tan(fovRad * 0.5f) * 1.5f;

				m_CameraController.GetCamera().SetPosition(center + Vector3(0.0f, 0.0f, distance));
				m_CameraController.GetCamera().SetRotation(0.0f, -90.0f);
				m_CameraController.SetNearClip(distance * 0.01f);
				m_CameraController.SetFarClip(distance * 10.0f);
			}
			else
			{
				m_CameraController.GetCamera().SetPosition({ 0.0f, 1.0f, 3.0f });
				m_CameraController.GetCamera().SetRotation(0.0f, -90.0f);
			}
			return;
		}

		const auto& vertices = m_ModelMesh->GetVertices();
		if (vertices.empty())
		{
			m_CameraController.GetCamera().SetPosition({ 0.0f, 1.0f, 3.0f });
			m_CameraController.GetCamera().SetRotation(0.0f, -90.0f);
			return;
		}

		// Compute AABB
		Vector3 boundsMin(std::numeric_limits<float>::max());
		Vector3 boundsMax(std::numeric_limits<float>::lowest());
		for (const auto& v : vertices)
		{
			boundsMin = glm::min(boundsMin, v.Position);
			boundsMax = glm::max(boundsMax, v.Position);
		}

		Vector3 center = (boundsMin + boundsMax) * 0.5f;
		Vector3 extents = boundsMax - boundsMin;
		float maxExtent = glm::max(extents.x, glm::max(extents.y, extents.z));
		if (maxExtent < 0.0001f) maxExtent = 1.0f;

		float fovRad = glm::radians(m_CameraController.GetFOV());
		float distance = (maxExtent * 0.5f) / std::tan(fovRad * 0.5f) * 1.5f;

		m_CameraController.GetCamera().SetPosition(center + Vector3(0.0f, 0.0f, distance));
		m_CameraController.GetCamera().SetRotation(0.0f, -90.0f);

		float nearClip = distance * 0.01f;
		if (nearClip < 0.001f) nearClip = 0.001f;
		m_CameraController.SetNearClip(nearClip);
		m_CameraController.SetFarClip(distance * 10.0f);
	}

	void VoxelTextureEditorPanel::OpenForAsset(UUID vtexUUID)
	{
		m_VtexUUID = vtexUUID;
		m_Vtex = AssetManager::GetAsset<VoxelTextureAsset>(vtexUUID);
		if (!m_Vtex)
		{
			CB_CORE_ERROR("VoxelTextureEditorPanel: Failed to load vtex asset");
			return;
		}

		m_IsOpen = true;
		m_Visible = true;

		// Load the source vmesh
		m_VmeshUUID = m_Vtex->SourceVmeshUUID;
		m_Vmesh = AssetManager::GetAsset<VoxelMeshAsset>(m_VmeshUUID);

		m_ModelMesh = nullptr;
		m_MaterialMapMesh = nullptr;
		m_SlicedMesh = nullptr;
		m_HasPalette = false;
		m_MaterialMapDirty = true;
		m_SliceY = 0;
		m_PrevSliceY = -1;
		m_ShowSliceView = false;

		// Reset custom brush tracking
		m_CustomBrushPaletteIndices.clear();
		m_RemovedPaletteIndices.clear();

		// Reset 2D grid / brush state
		m_SelectedBrushIndex = -1;
		m_GridPanOffset = { 0.0f, 0.0f };
		m_GridZoom = 1.0f;
		m_SliceLookup.clear();
		m_SliceLookupDirty = true;
		m_HoveredCell = { -1, -1 };
		m_IsPainting = false;

		// Reset color groups
		m_ColorGroupsDirty = true;
		m_ColorGroups.clear();

		if (m_Vmesh && m_Vmesh->VoxelCount > 0)
		{
			// Apply saved custom brushes to the vmesh palette
			if (m_Vmesh->HasPalette && !m_Vtex->CustomBrushes.empty())
			{
				for (const auto& brush : m_Vtex->CustomBrushes)
				{
					uint8_t newIdx = m_Vmesh->Palette.AddEntry(brush.Entry);
					m_CustomBrushPaletteIndices.push_back(newIdx);
					m_Vtex->SetPaletteType(newIdx, brush.MaterialType);
				}
			}

			// Apply saved palette index overrides
			if (m_Vmesh->HasPalette && !m_Vtex->PaletteIndexOverrides.empty())
			{
				for (const auto& [filledIdx, palIdx] : m_Vtex->PaletteIndexOverrides)
				{
					if (filledIdx < m_Vmesh->PaletteIndices.size())
						m_Vmesh->PaletteIndices[filledIdx] = palIdx;
				}
			}

			// Build model preview mesh with real palette colors
			if (m_Vmesh->HasPalette && !m_Vmesh->PaletteIndices.empty())
			{
				m_ModelMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(m_Vmesh->GridData, m_Vmesh->PaletteIndices);

				std::vector<uint8_t> colorData, materialData;
				m_Vmesh->Palette.GenerateColorTextureData(colorData);
				m_Vmesh->Palette.GenerateMaterialTextureData(materialData);

				m_PaletteColorTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
				m_PaletteMaterialTexture = Texture2D::CreateFromData(256, 1, materialData.data(), true);
				m_HasPalette = true;
			}
			else
			{
				m_ModelMesh = VoxelizerAPI::CreateMeshFromGrid(m_Vmesh->GridData);
				m_HasPalette = false;
			}

			// Fit camera to the loaded mesh
			FitCameraToMesh();

			// Build material map mesh
			RebuildMaterialMapMesh();

			// Set slice max
			m_SliceY = m_Vtex->GridSize.y - 1;
		}

		CB_CORE_INFO("VoxelTextureEditorPanel: Opened for vtex (vmesh: {0})", static_cast<uint64_t>(m_VmeshUUID));
	}

	void VoxelTextureEditorPanel::RebuildMaterialMapMesh()
	{
		if (!m_Vmesh || !m_Vtex || !m_Vmesh->HasPalette)
		{
			m_MaterialMapMesh = m_ModelMesh;
			m_MaterialMapPaletteTexture = nullptr;
			m_MaterialMapDirty = false;
			return;
		}

		// Create a palette texture where each entry is colored by material type
		VoxelPalette materialPalette;
		uint32_t usedCount = m_Vmesh->Palette.GetUsedCount();
		for (uint8_t i = 0; i < usedCount; i++)
		{
			VoxelMaterialType type = m_Vtex->GetMaterialType(0, i);
			Vector3 displayColor = VoxelMaterialTypeDisplayColor(type);

			VoxelPaletteEntry entry;
			entry.Color = displayColor;
			entry.Metallic = 0.0f;
			entry.Roughness = 0.7f;
			entry.Emission = 0.0f;

			if (i == 0)
				materialPalette.SetEntry(0, entry);
			else
				materialPalette.AddEntry(entry);
		}

		std::vector<uint8_t> colorData, materialData;
		materialPalette.GenerateColorTextureData(colorData);
		materialPalette.GenerateMaterialTextureData(materialData);

		m_MaterialMapPaletteTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
		m_MaterialMapMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(m_Vmesh->GridData, m_Vmesh->PaletteIndices);

		m_MaterialMapDirty = false;

		// Also rebuild sliced mesh if in slice mode
		if (m_ShowSliceView)
		{
			m_PrevSliceY = -1; // Force rebuild
		}
	}

	void VoxelTextureEditorPanel::RebuildSliceLookup()
	{
		m_SliceLookup.clear();
		m_SliceLookupDirty = false;

		if (!m_Vmesh || !m_Vmesh->HasPalette)
			return;

		const auto& grid = m_Vmesh->GridData;
		int sizeX = grid.size.x;
		int sizeZ = grid.size.z;
		m_SliceLookup.resize(sizeX * sizeZ, -1);

		uint32_t filledIdx = 0;
		for (uint64_t i = 0; i < grid.totalVoxels; i++)
		{
			if (grid.IsFilled(i))
			{
				glm::ivec3 coord = grid.IndexToCoord(i);
				if (coord.y == m_SliceY)
					m_SliceLookup[coord.x * sizeZ + coord.z] = static_cast<int32_t>(filledIdx);
				filledIdx++;
			}
		}
	}

	void VoxelTextureEditorPanel::Render2DGridView()
	{
		if (!m_Vmesh || !m_Vtex)
		{
			ImGui::TextDisabled("No data for 2D grid");
			return;
		}

		const auto& grid = m_Vmesh->GridData;
		int sizeX = grid.size.x;
		int sizeZ = grid.size.z;
		if (sizeX <= 0 || sizeZ <= 0)
			return;

		// Lookup not ready yet (will be rebuilt next frame)
		if (static_cast<int>(m_SliceLookup.size()) != sizeX * sizeZ)
			return;

		ImVec2 avail = ImGui::GetContentRegionAvail();
		float baseCellSize = glm::min(avail.x / (float)sizeX, avail.y / (float)sizeZ);
		baseCellSize = std::max(baseCellSize, 1.0f);
		float cellSize = baseCellSize * m_GridZoom;

		float gridW = cellSize * sizeX;
		float gridH = cellSize * sizeZ;

		// Center the grid in available space + pan offset
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		float offsetX = (avail.x - gridW) * 0.5f + m_GridPanOffset.x;
		float offsetY = (avail.y - gridH) * 0.5f + m_GridPanOffset.y;

		ImVec2 gridOrigin = ImVec2(cursorPos.x + offsetX, cursorPos.y + offsetY);

		// Invisible button to capture input over the full area
		ImGui::InvisibleButton("##2DGrid", avail, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
		bool isHovered = ImGui::IsItemHovered();
		bool isActive = ImGui::IsItemActive();

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Zoom with scroll wheel
		if (isHovered)
		{
			float scroll = ImGui::GetIO().MouseWheel;
			if (scroll != 0.0f)
			{
				ImVec2 mousePos = ImGui::GetIO().MousePos;
				// Zoom toward cursor
				float oldZoom = m_GridZoom;
				m_GridZoom *= (scroll > 0.0f) ? 1.15f : (1.0f / 1.15f);
				m_GridZoom = glm::clamp(m_GridZoom, 0.5f, 20.0f);

				// Adjust pan so the point under cursor stays in place
				float zoomRatio = m_GridZoom / oldZoom;
				float mx = mousePos.x - cursorPos.x;
				float my = mousePos.y - cursorPos.y;
				m_GridPanOffset.x = mx - zoomRatio * (mx - m_GridPanOffset.x - (avail.x - baseCellSize * oldZoom * sizeX) * 0.5f)
					- (avail.x - baseCellSize * m_GridZoom * sizeX) * 0.5f;
				m_GridPanOffset.y = my - zoomRatio * (my - m_GridPanOffset.y - (avail.y - baseCellSize * oldZoom * sizeZ) * 0.5f)
					- (avail.y - baseCellSize * m_GridZoom * sizeZ) * 0.5f;

				// Recompute after zoom change
				cellSize = baseCellSize * m_GridZoom;
				gridW = cellSize * sizeX;
				gridH = cellSize * sizeZ;
				offsetX = (avail.x - gridW) * 0.5f + m_GridPanOffset.x;
				offsetY = (avail.y - gridH) * 0.5f + m_GridPanOffset.y;
				gridOrigin = ImVec2(cursorPos.x + offsetX, cursorPos.y + offsetY);
			}
		}

		// Pan with middle-mouse drag
		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
		{
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			m_GridPanOffset.x += delta.x;
			m_GridPanOffset.y += delta.y;
			gridOrigin = ImVec2(cursorPos.x + (avail.x - gridW) * 0.5f + m_GridPanOffset.x,
				cursorPos.y + (avail.y - gridH) * 0.5f + m_GridPanOffset.y);
		}

		// Compute hovered cell
		m_HoveredCell = { -1, -1 };
		if (isHovered)
		{
			ImVec2 mousePos = ImGui::GetIO().MousePos;
			int cx = static_cast<int>((mousePos.x - gridOrigin.x) / cellSize);
			int cz = static_cast<int>((mousePos.y - gridOrigin.y) / cellSize);
			if (cx >= 0 && cx < sizeX && cz >= 0 && cz < sizeZ)
				m_HoveredCell = { cx, cz };
		}

		// Draw cells
		uint32_t usedCount = m_Vmesh->Palette.GetUsedCount();
		for (int x = 0; x < sizeX; x++)
		{
			for (int z = 0; z < sizeZ; z++)
			{
				ImVec2 pMin(gridOrigin.x + x * cellSize, gridOrigin.y + z * cellSize);
				ImVec2 pMax(pMin.x + cellSize, pMin.y + cellSize);

				int32_t filledIdx = m_SliceLookup[x * sizeZ + z];
				if (filledIdx >= 0)
				{
					// Filled voxel — show material type display color
					uint8_t palIdx = 0;
					if (filledIdx < static_cast<int32_t>(m_Vmesh->PaletteIndices.size()))
						palIdx = m_Vmesh->PaletteIndices[filledIdx];

					VoxelMaterialType type = m_Vtex->GetMaterialType(filledIdx, palIdx);
					Vector3 col = VoxelMaterialTypeDisplayColor(type);
					drawList->AddRectFilled(pMin, pMax, IM_COL32((int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255), 255));

					// Small palette color indicator in top-left corner when zoomed in
					if (cellSize >= 12.0f)
					{
						const auto& entry = m_Vmesh->Palette.GetEntry(palIdx);
						float indicatorSize = glm::min(cellSize * 0.3f, 8.0f);
						ImVec2 iMax(pMin.x + indicatorSize, pMin.y + indicatorSize);
						drawList->AddRectFilled(pMin, iMax,
							IM_COL32(static_cast<int>(entry.Color.x * 255), static_cast<int>(entry.Color.y * 255), static_cast<int>(entry.Color.z * 255), 255));
					}
				}
				else
				{
					// Empty cell
					drawList->AddRectFilled(pMin, pMax, IM_COL32(30, 30, 35, 255));
				}
			}
		}

		// Grid lines when zoomed in enough
		if (cellSize >= 8.0f)
		{
			for (int x = 0; x <= sizeX; x++)
			{
				float px = gridOrigin.x + x * cellSize;
				drawList->AddLine(ImVec2(px, gridOrigin.y), ImVec2(px, gridOrigin.y + gridH), IM_COL32(60, 60, 70, 180));
			}
			for (int z = 0; z <= sizeZ; z++)
			{
				float pz = gridOrigin.y + z * cellSize;
				drawList->AddLine(ImVec2(gridOrigin.x, pz), ImVec2(gridOrigin.x + gridW, pz), IM_COL32(60, 60, 70, 180));
			}
		}

		// Hovered cell highlight
		if (m_HoveredCell.x >= 0 && m_HoveredCell.y >= 0)
		{
			int32_t filledIdx = m_SliceLookup[m_HoveredCell.x * sizeZ + m_HoveredCell.y];
			ImVec2 pMin(gridOrigin.x + m_HoveredCell.x * cellSize, gridOrigin.y + m_HoveredCell.y * cellSize);
			ImVec2 pMax(pMin.x + cellSize, pMin.y + cellSize);
			ImU32 borderCol = (filledIdx >= 0) ? IM_COL32(255, 255, 0, 255) : IM_COL32(100, 100, 100, 180);
			drawList->AddRect(pMin, pMax, borderCol, 0.0f, 0, 2.0f);
		}

		HandleGridPainting();
	}

	void VoxelTextureEditorPanel::HandleGridPainting()
	{
		if (m_SelectedBrushIndex < 0)
			return;

		if (!m_Vmesh || !m_Vtex)
			return;

		const auto& grid = m_Vmesh->GridData;
		int sizeZ = grid.size.z;

		bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
		bool wasClicked = ImGui::IsItemActive() && mouseDown;

		if (wasClicked && m_HoveredCell.x >= 0 && m_HoveredCell.y >= 0)
		{
			int32_t filledIdx = m_SliceLookup[m_HoveredCell.x * sizeZ + m_HoveredCell.y];
			if (filledIdx >= 0 && filledIdx < static_cast<int32_t>(m_Vmesh->PaletteIndices.size()))
			{
				uint8_t currentPalIdx = m_Vmesh->PaletteIndices[filledIdx];
				if (currentPalIdx != static_cast<uint8_t>(m_SelectedBrushIndex))
				{
					m_Vmesh->PaletteIndices[filledIdx] = static_cast<uint8_t>(m_SelectedBrushIndex);
					m_Vtex->PaletteIndexOverrides[static_cast<uint32_t>(filledIdx)] = static_cast<uint8_t>(m_SelectedBrushIndex);
					m_MaterialMapDirty = true;
				}
			}
		}
	}

	void VoxelTextureEditorPanel::RebuildColorGroups()
	{
		m_ColorGroups.clear();
		m_ColorGroupsDirty = false;

		if (!m_Vmesh || !m_Vmesh->HasPalette || !m_Vtex)
			return;

		uint32_t usedCount = m_Vmesh->Palette.GetUsedCount();
		const float threshold = 0.40f; // Euclidean distance in [0,1] RGB space

		for (uint8_t i = 0; i < usedCount; i++)
		{
			// Skip removed entries
			if (m_RemovedPaletteIndices.count(i))
				continue;

			const auto& entry = m_Vmesh->Palette.GetEntry(i);
			Vector3 color = entry.Color;

			// Try to find an existing group within threshold
			int bestGroup = -1;
			float bestDist = threshold;
			for (int g = 0; g < static_cast<int>(m_ColorGroups.size()); g++)
			{
				float dist = glm::length(m_ColorGroups[g].AverageColor - color);
				if (dist < bestDist)
				{
					bestDist = dist;
					bestGroup = g;
				}
			}

			if (bestGroup >= 0)
			{
				// Add to existing group, update running average
				auto& group = m_ColorGroups[bestGroup];
				float n = static_cast<float>(group.EntryIndices.size());
				group.AverageColor = (group.AverageColor * n + color) / (n + 1.0f);
				group.EntryIndices.push_back(i);
			}
			else
			{
				// Create new group
				PaletteColorGroup group;
				group.AverageColor = color;
				group.EntryIndices.push_back(i);
				group.Expanded = false;

				// Assign material type: use existing palette mapping if all entries agree
				group.MaterialType = m_Vtex->GetMaterialType(0, i);
				m_ColorGroups.push_back(group);
			}
		}

		// For groups with multiple entries, check if all have same type; if not, auto-detect from average
		for (auto& group : m_ColorGroups)
		{
			if (group.EntryIndices.size() > 1)
			{
				VoxelMaterialType firstType = m_Vtex->GetMaterialType(0, group.EntryIndices[0]);
				bool allSame = true;
				for (size_t j = 1; j < group.EntryIndices.size(); j++)
				{
					if (m_Vtex->GetMaterialType(0, group.EntryIndices[j]) != firstType)
					{
						allSame = false;
						break;
					}
				}

				if (allSame)
					group.MaterialType = firstType;
				else
				{
					// Average metallic/roughness for auto-detect
					float avgMet = 0.0f, avgRough = 0.0f;
					for (uint8_t idx : group.EntryIndices)
					{
						const auto& e = m_Vmesh->Palette.GetEntry(idx);
						avgMet += e.Metallic;
						avgRough += e.Roughness;
					}
					float n = static_cast<float>(group.EntryIndices.size());
					group.MaterialType = AutoDetectMaterialType(group.AverageColor);

					// Sync group type back to all entries so brush selection is consistent
					for (uint8_t idx : group.EntryIndices)
						m_Vtex->SetPaletteType(idx, group.MaterialType);
					m_MaterialMapDirty = true;
				}
			}
		}
	}

	void VoxelTextureEditorPanel::RebuildSlicedMesh()
	{
		if (!m_Vmesh || !m_Vtex || !m_Vmesh->HasPalette)
		{
			m_SlicedMesh = nullptr;
			return;
		}

		// Build a new VoxelGrid with only voxels at y <= m_SliceY
		const auto& srcGrid = m_Vmesh->GridData;
		voxelizer::VoxelGrid slicedGrid;
		slicedGrid.size = srcGrid.size;
		slicedGrid.origin = srcGrid.origin;
		slicedGrid.voxelSize = srcGrid.voxelSize;
		slicedGrid.totalVoxels = srcGrid.totalVoxels;
		slicedGrid.data.resize(srcGrid.data.size(), 0);

		// Also build sliced palette indices
		std::vector<uint8_t> slicedPaletteIndices;

		// Iterate in linear index order (matching GetFilledCoords) to keep
		// palette indices in sync with the source grid
		uint32_t srcFilledIdx = 0;
		for (uint64_t i = 0; i < srcGrid.totalVoxels; i++)
		{
			if (srcGrid.IsFilled(i))
			{
				glm::ivec3 coord = srcGrid.IndexToCoord(i);
				if (coord.y <= m_SliceY)
				{
					slicedGrid.SetFilled(i);
					if (srcFilledIdx < m_Vmesh->PaletteIndices.size())
						slicedPaletteIndices.push_back(m_Vmesh->PaletteIndices[srcFilledIdx]);
					else
						slicedPaletteIndices.push_back(0);
				}
				srcFilledIdx++;
			}
		}

		// Build material type palette texture for the sliced mesh
		VoxelPalette materialPalette;
		uint32_t usedCount = m_Vmesh->Palette.GetUsedCount();
		for (uint8_t i = 0; i < usedCount; i++)
		{
			VoxelMaterialType type = m_Vtex->GetMaterialType(0, i);
			Vector3 displayColor = VoxelMaterialTypeDisplayColor(type);

			VoxelPaletteEntry entry;
			entry.Color = displayColor;
			entry.Metallic = 0.0f;
			entry.Roughness = 0.7f;

			if (i == 0)
				materialPalette.SetEntry(0, entry);
			else
				materialPalette.AddEntry(entry);
		}

		std::vector<uint8_t> colorData, materialData;
		materialPalette.GenerateColorTextureData(colorData);
		materialPalette.GenerateMaterialTextureData(materialData);

		m_SlicedPaletteTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
		m_SlicedMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(slicedGrid, slicedPaletteIndices);

		m_PrevSliceY = m_SliceY;
	}

	void VoxelTextureEditorPanel::OnUpdate(Timestep ts)
	{
		if (!m_Visible)
			return;

		if (m_MaterialMapDirty)
		{
			RebuildMaterialMapMesh();

			// Rebuild model preview mesh with updated palette indices (painting changes them)
			if (m_Vmesh && m_Vmesh->HasPalette && !m_Vmesh->PaletteIndices.empty())
				m_ModelMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(m_Vmesh->GridData, m_Vmesh->PaletteIndices);

			m_SliceLookupDirty = true;
		}

		// Rebuild sliced mesh if layer changed (only needed for 3D sliced view)
		if (m_ShowSliceView && m_SliceY != m_PrevSliceY)
		{
			RebuildSlicedMesh();
			m_SliceLookupDirty = true;
		}

		// Rebuild 2D slice lookup when needed
		if (m_ShowSliceView && m_SliceLookupDirty)
			RebuildSliceLookup();

		// Update camera from either viewport
		if (m_ModelViewportHovered || m_MaterialMapViewportHovered)
			m_CameraController.OnUpdate(ts);

		// Auto-rotate only if no viewport is hovered
		if (!m_ModelViewportHovered && !m_MaterialMapViewportHovered)
		{
			m_AutoRotationAngle += ts * 30.0f;
			if (m_AutoRotationAngle >= 360.0f)
				m_AutoRotationAngle -= 360.0f;
		}

		// Resize model framebuffer
		{
			auto spec = m_ModelFramebuffer->GetSpecification();
			if (m_ModelViewportSize.x > 0 && m_ModelViewportSize.y > 0)
			{
				if (spec.Width != static_cast<uint32_t>(m_ModelViewportSize.x) ||
					spec.Height != static_cast<uint32_t>(m_ModelViewportSize.y))
				{
					m_ModelFramebuffer->Resize(
						static_cast<uint32_t>(m_ModelViewportSize.x),
						static_cast<uint32_t>(m_ModelViewportSize.y));
					m_CameraController.SetViewportSize(m_ModelViewportSize.x, m_ModelViewportSize.y);
				}
			}
		}

		// Resize material map framebuffer
		{
			auto spec = m_MaterialMapFramebuffer->GetSpecification();
			if (m_MaterialMapViewportSize.x > 0 && m_MaterialMapViewportSize.y > 0)
			{
				if (spec.Width != static_cast<uint32_t>(m_MaterialMapViewportSize.x) ||
					spec.Height != static_cast<uint32_t>(m_MaterialMapViewportSize.y))
				{
					m_MaterialMapFramebuffer->Resize(
						static_cast<uint32_t>(m_MaterialMapViewportSize.x),
						static_cast<uint32_t>(m_MaterialMapViewportSize.y));
				}
			}
		}

		Mat4 rotation = glm::rotate(Mat4(1.0f), glm::radians(m_AutoRotationAngle), Vector3(0.0f, 1.0f, 0.0f));

		// Render model preview
		if (m_ModelMesh)
		{
			m_ModelFramebuffer->Bind();
			RenderCommand::SetClearColor({ 0.15f, 0.15f, 0.18f, 1.0f });
			RenderCommand::Clear();

			Renderer3D::BeginScene(m_CameraController.GetCamera(), m_CameraController.GetCamera().GetPosition());
			Renderer3D::SetDirectionalLight(Vector3(-0.5f, -1.0f, -0.3f), Vector3(1.0f), 1.5f);
			Renderer3D::SetAmbientLight(Vector3(0.15f));
			Renderer3D::Submit(m_PreviewShader, m_PreviewMaterial, m_ModelMesh, rotation,
				-1, false, m_PaletteColorTexture, m_PaletteMaterialTexture);
			Renderer3D::EndScene();

			m_ModelFramebuffer->Unbind();
		}

		// Render material map (skip 3D rendering when in 2D grid mode)
		if (!m_ShowSliceView)
		{
			if (m_MaterialMapMesh)
			{
				m_MaterialMapFramebuffer->Bind();
				RenderCommand::SetClearColor({ 0.12f, 0.12f, 0.15f, 1.0f });
				RenderCommand::Clear();

				Renderer3D::BeginScene(m_CameraController.GetCamera(), m_CameraController.GetCamera().GetPosition());
				Renderer3D::SetDirectionalLight(Vector3(-0.5f, -1.0f, -0.3f), Vector3(1.0f), 1.5f);
				Renderer3D::SetAmbientLight(Vector3(0.15f));
				Renderer3D::Submit(m_PreviewShader, m_PreviewMaterial, m_MaterialMapMesh, rotation,
					-1, false, m_MaterialMapPaletteTexture, nullptr);
				Renderer3D::EndScene();

				m_MaterialMapFramebuffer->Unbind();
			}
		}
	}

	void VoxelTextureEditorPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Voxel Texture Editor", &m_Visible))
		{
			// Asset picker at the top
			UUID prevUUID = m_VtexUUID;
			if (AssetPicker::DrawVoxelTexture("Voxel Texture", m_VtexUUID))
			{
				if (m_VtexUUID.IsValid() && m_VtexUUID != prevUUID)
					OpenForAsset(m_VtexUUID);
				else if (!m_VtexUUID.IsValid())
				{
					m_Vtex = nullptr;
					m_Vmesh = nullptr;
					m_ModelMesh = nullptr;
					m_MaterialMapMesh = nullptr;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("New from VMesh..."))
				ImGui::OpenPopup("GenerateVtexPopup");

			if (ImGui::BeginPopup("GenerateVtexPopup"))
			{
				ImGui::Text("Select a VoxelMesh to generate from:");
				ImGui::Separator();

				static UUID pickerVmeshUUID = UUID(0);
				AssetPicker::DrawVoxelMesh("Source VMesh", pickerVmeshUUID);

				if (pickerVmeshUUID.IsValid())
				{
					if (ImGui::Button("Generate .vtex"))
					{
						auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(pickerVmeshUUID);
						if (vmesh)
						{
							auto newVtex = VoxelTextureAsset::GenerateFromVmesh(vmesh);
							if (newVtex)
							{
								const AssetMetadata* vmeta = AssetManager::GetRegistry().GetMetadata(pickerVmeshUUID);
								if (vmeta)
								{
									std::filesystem::path vmeshFullPath = AssetManager::GetAssetDirectory() / vmeta->FilePath;
									std::filesystem::path vtexPath = vmeshFullPath;
									vtexPath.replace_extension(".vtex");

									if (newVtex->Save(vtexPath))
									{
										std::filesystem::path relVtex = std::filesystem::relative(
											vtexPath, AssetManager::GetAssetDirectory());
										UUID vtexUUID = AssetManager::ImportAsset(relVtex);
										if (vtexUUID.IsValid())
										{
											pickerVmeshUUID = UUID(0);
											ImGui::CloseCurrentPopup();
											OpenForAsset(vtexUUID);
										}
									}
								}
							}
						}
					}
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			if (!m_Vtex)
			{
				ImGui::TextDisabled("Select a .vtex asset above, or click 'New from VMesh...' to generate one");
			}
			else
			{
				float availWidth = ImGui::GetContentRegionAvail().x;
				float spacing = ImGui::GetStyle().ItemSpacing.x;
				float columnWidth = (availWidth - spacing * 2.0f) / 3.0f;
				float viewportHeight = ImGui::GetContentRegionAvail().y;

				// Left column: Model Preview
				ImGui::BeginChild("##ModelPreview", ImVec2(columnWidth, viewportHeight), true);
				{
					ImGui::Text("Model Preview");
					ImGui::Separator();
					RenderModelPreview();
				}
				ImGui::EndChild();

				ImGui::SameLine();

				// Middle column: Material Map / Layer View
				ImGui::BeginChild("##MaterialMap", ImVec2(columnWidth, viewportHeight), true);
				{
					bool prevSliceView = m_ShowSliceView;
					ImGui::Checkbox("Layer View", &m_ShowSliceView);
					if (m_ShowSliceView && !prevSliceView)
					{
						// Just toggled on — reset 2D view state
						m_GridPanOffset = { 0.0f, 0.0f };
						m_GridZoom = 1.0f;
						m_SliceLookupDirty = true;
					}
					if (m_ShowSliceView)
					{
						ImGui::SameLine();
						int maxY = m_Vtex->GridSize.y - 1;
						if (maxY < 0) maxY = 0;
						int prevY = m_SliceY;
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
						ImGui::SliderInt("##YLayer", &m_SliceY, 0, maxY, "Y: %d");
						if (m_SliceY != prevY)
							m_SliceLookupDirty = true;
					}
					ImGui::Separator();
					RenderMaterialMapView();
				}
				ImGui::EndChild();

				ImGui::SameLine();

				// Right column: Palette Editor
				ImGui::BeginChild("##PaletteEditor", ImVec2(columnWidth, viewportHeight), true);
				{
					ImGui::Text("Palette Editor");
					ImGui::Separator();
					RenderPaletteEditor();
				}
				ImGui::EndChild();
			}
		}
		ImGui::End();
	}

	void VoxelTextureEditorPanel::OnEvent(Event& e)
	{
		if (!m_Visible)
			return;

		if (m_ModelViewportHovered || m_MaterialMapViewportHovered)
			m_CameraController.OnEvent(e);
	}

	void VoxelTextureEditorPanel::RenderModelPreview()
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		m_ModelViewportSize = { size.x, size.y };

		if (m_ModelMesh)
		{
			uint64_t texID = m_ModelFramebuffer->GetColorAttachmentRendererID();
			ImGui::Image(reinterpret_cast<void*>(texID), size, ImVec2(0, 1), ImVec2(1, 0));
			m_ModelViewportHovered = ImGui::IsItemHovered();
		}
		else
		{
			ImGui::TextDisabled("No vmesh data");
			m_ModelViewportHovered = false;
		}
	}

	void VoxelTextureEditorPanel::RenderMaterialMapView()
	{
		if (m_ShowSliceView)
		{
			// 2D top-down grid view — no 3D camera interaction
			m_MaterialMapViewportHovered = false;
			Render2DGridView();
			return;
		}

		ImVec2 size = ImGui::GetContentRegionAvail();
		m_MaterialMapViewportSize = { size.x, size.y };

		if (m_MaterialMapMesh)
		{
			uint64_t texID = m_MaterialMapFramebuffer->GetColorAttachmentRendererID();
			ImGui::Image(reinterpret_cast<void*>(texID), size, ImVec2(0, 1), ImVec2(1, 0));
			m_MaterialMapViewportHovered = ImGui::IsItemHovered();
		}
		else
		{
			ImGui::TextDisabled("No material map");
			m_MaterialMapViewportHovered = false;
		}
	} 

	void VoxelTextureEditorPanel::RenderPaletteEditor()
	{
		if (!m_Vtex || !m_Vmesh)
		{
			ImGui::TextDisabled("No data");
			return;
		}

		// Material type names for combo
		static const char* materialTypeNames[] = { "Stone", "Wood", "Metal", "Glass", "Marble" };
		static_assert(std::size(materialTypeNames) == static_cast<int>(VoxelMaterialType::Count));

		// Row 1: Auto-Detect + Save
		if (ImGui::Button("Auto-Detect All"))
		{
			if (m_Vmesh->HasPalette)
			{
				m_Vtex->PaletteMapping.clear();
				uint32_t usedCount = m_Vmesh->Palette.GetUsedCount();
				for (uint8_t i = 0; i < usedCount; i++)
				{
					const auto& entry = m_Vmesh->Palette.GetEntry(i);
					m_Vtex->PaletteMapping[i] = AutoDetectMaterialType(entry.Color);
				}
				m_MaterialMapDirty = true;
				m_ColorGroupsDirty = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Save"))
		{
			if (m_Vtex->GetPath().empty())
			{
				const AssetMetadata* vmeta = AssetManager::GetRegistry().GetMetadata(m_VmeshUUID);
				if (vmeta)
				{
					std::filesystem::path vmeshPath = AssetManager::GetAssetDirectory() / vmeta->FilePath;
					std::filesystem::path vtexPath = vmeshPath;
					vtexPath.replace_extension(".vtex");
					m_Vtex->Save(vtexPath);
				}
			}
			else
			{
				m_Vtex->Save(m_Vtex->GetPath());
			}
		}

		ImGui::Separator();

		// Generate from Texture
		ImGui::Text("Generate from Texture:");
		AssetPicker::DrawTexture("Source Texture", m_SourceTextureUUID);
		if (m_SourceTextureUUID.IsValid())
		{
			if (ImGui::Button("Generate Mapping"))
			{
				const AssetMetadata* texMeta = AssetManager::GetRegistry().GetMetadata(m_SourceTextureUUID);
				if (texMeta)
				{
					std::filesystem::path texPath = AssetManager::GetAssetDirectory() / texMeta->FilePath;
					m_Vtex->GenerateMappingFromTexture(m_Vmesh, texPath);
					m_MaterialMapDirty = true;
					m_ColorGroupsDirty = true;
				}
			}
		}

		ImGui::Separator();

		// Brush indicator
		if (m_SelectedBrushIndex >= 0 && m_SelectedBrushIndex < (int)m_Vmesh->Palette.GetUsedCount())
		{
			const auto& brushEntry = m_Vmesh->Palette.GetEntry((uint8_t)m_SelectedBrushIndex);
			VoxelMaterialType brushType = m_Vtex->GetMaterialType(0, (uint8_t)m_SelectedBrushIndex);
			Vector3 brushMtCol = VoxelMaterialTypeDisplayColor(brushType);

			ImGui::Text("Brush:");
			ImGui::SameLine();
			// Material type color (what the cell will look like)
			ImGui::ColorButton("##brushmtcol", ImVec4(brushMtCol.x, brushMtCol.y, brushMtCol.z, 1.0f),
				ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16));
			ImGui::SameLine();
			// Actual palette color (small indicator)
			ImGui::ColorButton("##brushcol", ImVec4(brushEntry.Color.r, brushEntry.Color.g, brushEntry.Color.b, 1.0f),
				ImGuiColorEditFlags_NoTooltip, ImVec2(10, 10));
			ImGui::SameLine();
			ImGui::Text("%s [%d]", VoxelMaterialTypeToString(brushType), m_SelectedBrushIndex);
			ImGui::SameLine();
			if (ImGui::SmallButton("Clear"))
				m_SelectedBrushIndex = -1;
		}
		else
		{
			ImGui::TextDisabled("No brush selected (click a color below)");
		}

		ImGui::Separator();

		if (!m_Vmesh->HasPalette)
		{
			ImGui::TextDisabled("No palette data in vmesh");
			return;
		}

		uint32_t usedCount = m_Vmesh->Palette.GetUsedCount();

		// Toggle grouped/individual view
		bool prevGrouped = m_ShowGroupedView;
		ImGui::Checkbox("Group Colors", &m_ShowGroupedView);
		if (m_ShowGroupedView && (!prevGrouped || m_ColorGroupsDirty))
			RebuildColorGroups();

		if (m_ShowGroupedView)
		{
			// ---- Grouped view ----
			ImGui::Text("Color Groups (%u):", (uint32_t)m_ColorGroups.size());

			ImGui::BeginChild("##GroupedPaletteList", ImVec2(0, ImGui::GetContentRegionAvail().y - 120.0f), true);

			for (int g = 0; g < (int)m_ColorGroups.size(); g++)
			{
				auto& group = m_ColorGroups[g];
				ImGui::PushID(g);

				// Check if brush is in this group
				bool groupHasBrush = false;
				for (uint8_t idx : group.EntryIndices)
				{
					if ((int)idx == m_SelectedBrushIndex)
					{
						groupHasBrush = true;
						break;
					}
				}

				// Average color swatch (clickable to select first entry as brush)
				if (ImGui::ColorButton("##grpcol", ImVec4(group.AverageColor.r, group.AverageColor.g, group.AverageColor.b, 1.0f),
					ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20)))
				{
					m_SelectedBrushIndex = group.EntryIndices[0];
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Click to select as brush");
				ImGui::SameLine();

				// Material type combo
				int currentIdx = static_cast<int>(group.MaterialType);
				ImGui::SetNextItemWidth(70.0f);
				if (ImGui::Combo("##grptype", &currentIdx, materialTypeNames, (int)VoxelMaterialType::Count))
				{
					VoxelMaterialType newType = static_cast<VoxelMaterialType>(currentIdx);
					group.MaterialType = newType;
					for (uint8_t idx : group.EntryIndices)
						m_Vtex->SetPaletteType(idx, newType);
					m_MaterialMapDirty = true;
				}
				ImGui::SameLine();

				// Count badge + expand/collapse
				char label[64];
				snprintf(label, sizeof(label), "(%u)%s", (uint32_t)group.EntryIndices.size(),
					groupHasBrush ? " [B]" : "");
				if (ImGui::SmallButton(label))
					group.Expanded = !group.Expanded;

				// Expanded: show individual entries
				if (group.Expanded)
				{
					ImGui::Indent(20.0f);
					uint8_t entryToRemove = 255;
					for (uint8_t idx : group.EntryIndices)
					{
						ImGui::PushID(1000 + idx);
						const auto& entry = m_Vmesh->Palette.GetEntry(idx);
						bool isBrush = ((int)idx == m_SelectedBrushIndex);
						bool isCustom = IsCustomBrush(idx);

						if (isBrush)
						{
							ImVec2 cpMin = ImGui::GetCursorScreenPos();
							ImVec2 cpMax(cpMin.x + ImGui::GetContentRegionAvail().x, cpMin.y + 22.0f);
							ImGui::GetWindowDrawList()->AddRectFilled(cpMin, cpMax, IM_COL32(80, 80, 0, 60));
						}

						if (ImGui::ColorButton("##ecol", ImVec4(entry.Color.r, entry.Color.g, entry.Color.b, 1.0f),
							ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16)))
						{
							m_SelectedBrushIndex = idx;
						}
						ImGui::SameLine();

						VoxelMaterialType entryType = m_Vtex->GetMaterialType(0, idx);
						Vector3 mtCol = VoxelMaterialTypeDisplayColor(entryType);
						ImGui::ColorButton("##emtcol", ImVec4(mtCol.x, mtCol.y, mtCol.z, 1.0f),
							ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(10, 10));
						ImGui::SameLine();
						ImGui::Text("[%u]%s", idx, isBrush ? " [B]" : "");

						if (idx > 0)
						{
							ImGui::SameLine();
							if (ImGui::SmallButton("X"))
								entryToRemove = idx;
						}

						ImGui::PopID();
					}
					if (entryToRemove != 255)
						RemoveBrush(entryToRemove);
					ImGui::Unindent(20.0f);
				}

				ImGui::PopID();
			}

			ImGui::EndChild();
		}
		else
		{
			// ---- Individual view ----
			ImGui::Text("Palette Entries (%u):", usedCount);

			ImGui::BeginChild("##PaletteList", ImVec2(0, ImGui::GetContentRegionAvail().y - 120.0f), true);

			uint8_t individualRemove = 255;
			for (uint8_t i = 0; i < usedCount; i++)
			{
				// Skip removed entries
				if (m_RemovedPaletteIndices.count(i))
					continue;

				ImGui::PushID(i);

				const auto& entry = m_Vmesh->Palette.GetEntry(i);
				bool isBrush = ((int)i == m_SelectedBrushIndex);

				if (isBrush)
				{
					ImVec2 cpMin = ImGui::GetCursorScreenPos();
					ImVec2 cpMax(cpMin.x + ImGui::GetContentRegionAvail().x, cpMin.y + 22.0f);
					ImGui::GetWindowDrawList()->AddRectFilled(cpMin, cpMax, IM_COL32(80, 80, 0, 60));
				}

				// Color swatch (clickable as brush)
				if (ImGui::ColorButton("##color", ImVec4(entry.Color.r, entry.Color.g, entry.Color.b, 1.0f),
					ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20)))
				{
					m_SelectedBrushIndex = i;
				}
				ImGui::SameLine();

				// Material type combo
				VoxelMaterialType currentType = m_Vtex->GetMaterialType(0, i);
				int currentIdx = static_cast<int>(currentType);

				ImGui::SetNextItemWidth(80.0f);
				if (ImGui::Combo("##type", &currentIdx, materialTypeNames,
					(int)VoxelMaterialType::Count))
				{
					m_Vtex->SetPaletteType(i, static_cast<VoxelMaterialType>(currentIdx));
					m_MaterialMapDirty = true;
				}

				// Show material type color indicator
				ImGui::SameLine();
				Vector3 mtCol = VoxelMaterialTypeDisplayColor(currentType);
				ImGui::ColorButton("##mtcol", ImVec4(mtCol.x, mtCol.y, mtCol.z, 1.0f),
					ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(12, 12));

				if (isBrush)
				{
					ImGui::SameLine();
					ImGui::Text("[B]");
				}

				if (i > 0)
				{
					ImGui::SameLine();
					if (ImGui::SmallButton("X"))
						individualRemove = i;
				}

				ImGui::PopID();
			}
			if (individualRemove != 255)
				RemoveBrush(individualRemove);

			ImGui::EndChild();
		}

		// Add new palette entry
		ImGui::Separator();
		ImGui::Text("Add Entry:");
		ImGui::ColorEdit3("##newcolor", m_NewPaletteColor, ImGuiColorEditFlags_NoInputs);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::Combo("##newtype", &m_NewPaletteMaterialType, materialTypeNames, (int)VoxelMaterialType::Count);
		ImGui::SameLine();
		if (ImGui::Button("Add"))
		{
			if (usedCount < VoxelPalette::MaxEntries)
			{
				VoxelPaletteEntry newEntry;
				newEntry.Color = Vector3(m_NewPaletteColor[0], m_NewPaletteColor[1], m_NewPaletteColor[2]);
				newEntry.Metallic = 0.0f;
				newEntry.Roughness = 0.5f;
				uint8_t newIdx = m_Vmesh->Palette.AddEntry(newEntry);
				VoxelMaterialType newType = static_cast<VoxelMaterialType>(m_NewPaletteMaterialType);
				m_Vtex->SetPaletteType(newIdx, newType);

				// Record as custom brush for persistence
				CustomBrush brush;
				brush.Entry = newEntry;
				brush.MaterialType = newType;
				m_Vtex->CustomBrushes.push_back(brush);
				m_CustomBrushPaletteIndices.push_back(newIdx);

				m_MaterialMapDirty = true;
				m_ColorGroupsDirty = true;

				// Rebuild model preview palette textures
				std::vector<uint8_t> colorData, materialData;
				m_Vmesh->Palette.GenerateColorTextureData(colorData);
				m_Vmesh->Palette.GenerateMaterialTextureData(materialData);
				m_PaletteColorTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
				m_PaletteMaterialTexture = Texture2D::CreateFromData(256, 1, materialData.data(), true);
			}
		}

		ImGui::Separator();

		// Material type count summary
		uint32_t counts[(int)VoxelMaterialType::Count] = {};
		for (const auto& [idx, type] : m_Vtex->PaletteMapping)
			counts[(int)type]++;

		for (int i = 0; i < (int)VoxelMaterialType::Count; i++)
		{
			if (counts[i] > 0)
			{
				VoxelMaterialType mt = static_cast<VoxelMaterialType>(i);
				Vector3 col = VoxelMaterialTypeDisplayColor(mt);
				ImGui::ColorButton(("##sum" + std::to_string(i)).c_str(),
					ImVec4(col.x, col.y, col.z, 1.0f),
					ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
					ImVec2(10, 10));
				ImGui::SameLine();
				ImGui::Text("%s: %u", VoxelMaterialTypeToString(mt), counts[i]);
				ImGui::SameLine();
			}
		}
		ImGui::NewLine();
	}

	bool VoxelTextureEditorPanel::IsCustomBrush(uint8_t paletteIndex) const
	{
		for (uint8_t idx : m_CustomBrushPaletteIndices)
		{
			if (idx == paletteIndex)
				return true;
		}
		return false;
	}

	void VoxelTextureEditorPanel::RemoveBrush(uint8_t paletteIndex)
	{
		if (!m_Vmesh || !m_Vtex)
			return;

		// Revert all voxels painted with this brush back to palette index 0
		for (size_t i = 0; i < m_Vmesh->PaletteIndices.size(); i++)
		{
			if (m_Vmesh->PaletteIndices[i] == paletteIndex)
				m_Vmesh->PaletteIndices[i] = 0;
		}

		// Remove matching entries from PaletteIndexOverrides
		for (auto it = m_Vtex->PaletteIndexOverrides.begin(); it != m_Vtex->PaletteIndexOverrides.end(); )
		{
			if (it->second == paletteIndex)
				it = m_Vtex->PaletteIndexOverrides.erase(it);
			else
				++it;
		}

		// If it's a custom brush, remove from custom brush lists
		for (size_t i = 0; i < m_CustomBrushPaletteIndices.size(); i++)
		{
			if (m_CustomBrushPaletteIndices[i] == paletteIndex)
			{
				m_CustomBrushPaletteIndices.erase(m_CustomBrushPaletteIndices.begin() + i);
				m_Vtex->CustomBrushes.erase(m_Vtex->CustomBrushes.begin() + i);
				break;
			}
		}

		// Hide this entry from the palette UI
		m_RemovedPaletteIndices.insert(paletteIndex);

		// Clear brush selection if it was the removed brush
		if (m_SelectedBrushIndex == (int)paletteIndex)
			m_SelectedBrushIndex = -1;

		m_MaterialMapDirty = true;
		m_ColorGroupsDirty = true;

		// Rebuild palette textures
		std::vector<uint8_t> colorData, materialData;
		m_Vmesh->Palette.GenerateColorTextureData(colorData);
		m_Vmesh->Palette.GenerateMaterialTextureData(materialData);
		m_PaletteColorTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
		m_PaletteMaterialTexture = Texture2D::CreateFromData(256, 1, materialData.data(), true);
	}
}
