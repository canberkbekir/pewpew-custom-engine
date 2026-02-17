#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"

#include <Voxelizer.h>
#include <vector>

namespace CB
{
	/// A fragment produced by splitting a voxel grid into connected components
	struct VoxelFragment
	{
		/// Tight-fitting sub-grid containing only this fragment's voxels
		voxelizer::VoxelGrid Grid;
		/// Palette indices for each filled voxel in the sub-grid (same order as GetFilledCoords)
		std::vector<uint8_t> PaletteIndices;
		/// Center of mass in world space (average of filled voxel centers)
		Vector3 CenterOfMass;
		/// Number of filled voxels in this fragment
		uint64_t VoxelCount = 0;
		/// Offset from original grid origin to this fragment's grid origin (in voxel coords)
		glm::ivec3 MinCoord;
	};

	class VoxelSplitter
	{
	public:
		/// Remove all voxels within a sphere (world-space center + radius).
		/// Returns the updated voxel count.
		static uint64_t RemoveSphere(voxelizer::VoxelGrid& grid,
			std::vector<uint8_t>& paletteIndices,
			const Vector3& worldCenter, float radius);

		/// Remove specific voxels by coordinate list.
		/// Returns the updated voxel count.
		static uint64_t RemoveVoxels(voxelizer::VoxelGrid& grid,
			std::vector<uint8_t>& paletteIndices,
			const std::vector<glm::ivec3>& coordsToRemove);

		/// Find connected components via 6-connected BFS flood fill.
		/// Returns a vector of VoxelFragment, one per connected component.
		/// If the grid is fully connected, returns a single fragment.
		static std::vector<VoxelFragment> FindConnectedComponents(
			const voxelizer::VoxelGrid& grid,
			const std::vector<uint8_t>& paletteIndices);

	private:
		/// Extract a tight-fitting sub-grid for one connected component.
		static VoxelFragment ExtractComponent(
			const voxelizer::VoxelGrid& sourceGrid,
			const std::vector<uint8_t>& sourcePaletteIndices,
			const std::vector<glm::ivec3>& componentCoords);

		/// Build a mapping from filled-voxel linear index to palette index position.
		static std::unordered_map<uint64_t, size_t> BuildFilledIndexMap(
			const voxelizer::VoxelGrid& grid);
	};
}
