#include "cbpch.h"
#include "VoxelSplitter.h"

namespace CB
{
	std::unordered_map<uint64_t, size_t> VoxelSplitter::BuildFilledIndexMap(const voxelizer::VoxelGrid& grid)
	{
		// MUST iterate in linear index order (same as GetFilledCoords) so that
		// filledIdx matches the palette index positions used by
		// CreatePaletteMeshFromGrid. Linear index = x*sizeY*sizeZ + y*sizeZ + z.
		std::unordered_map<uint64_t, size_t> map;
		size_t filledIdx = 0;
		for (uint64_t i = 0; i < grid.totalVoxels; ++i)
		{
			if (grid.IsFilled(i))
				map[i] = filledIdx++;
		}
		return map;
	}

	uint64_t VoxelSplitter::RemoveSphere(voxelizer::VoxelGrid& grid,
		std::vector<uint8_t>& paletteIndices,
		const Vector3& worldCenter, float radius)
	{
		// Convert sphere center to voxel space
		glm::ivec3 centerVoxel = grid.WorldToVoxel(worldCenter);
		float radiusInVoxels = radius / glm::length(grid.voxelSize);

		// Compute bounding box in voxel coords
		int r = static_cast<int>(std::ceil(radiusInVoxels)) + 1;
		glm::ivec3 minCoord = glm::max(centerVoxel - glm::ivec3(r), glm::ivec3(0));
		glm::ivec3 maxCoord = glm::min(centerVoxel + glm::ivec3(r), grid.size - glm::ivec3(1));

		// Build filled index map for palette remapping
		auto filledMap = BuildFilledIndexMap(grid);

		// Collect voxels to remove
		std::vector<glm::ivec3> toRemove;
		for (int z = minCoord.z; z <= maxCoord.z; z++)
			for (int y = minCoord.y; y <= maxCoord.y; y++)
				for (int x = minCoord.x; x <= maxCoord.x; x++)
				{
					if (!grid.IsFilled(x, y, z))
						continue;

					Vector3 voxelWorld = grid.VoxelCenterToWorld({x, y, z});
					float dist = glm::distance(voxelWorld, worldCenter);
					if (dist <= radius)
						toRemove.push_back({x, y, z});
				}

		if (toRemove.empty())
			return grid.CountFilled();

		// Mark indices to remove from palette
		std::unordered_set<size_t> removedPaletteIndices;
		for (const auto& coord : toRemove)
		{
			uint64_t idx = grid.CoordToIndex(coord.x, coord.y, coord.z);
			auto it = filledMap.find(idx);
			if (it != filledMap.end())
				removedPaletteIndices.insert(it->second);
		}

		// Clear voxels from grid
		for (const auto& coord : toRemove)
			grid.Clear(coord.x, coord.y, coord.z);

		// Rebuild palette indices (remove entries for deleted voxels)
		if (!paletteIndices.empty())
		{
			std::vector<uint8_t> newPalette;
			newPalette.reserve(paletteIndices.size() - removedPaletteIndices.size());
			for (size_t i = 0; i < paletteIndices.size(); i++)
			{
				if (removedPaletteIndices.find(i) == removedPaletteIndices.end())
					newPalette.push_back(paletteIndices[i]);
			}
			paletteIndices = std::move(newPalette);
		}

		return grid.CountFilled();
	}

	uint64_t VoxelSplitter::RemoveVoxels(voxelizer::VoxelGrid& grid,
		std::vector<uint8_t>& paletteIndices,
		const std::vector<glm::ivec3>& coordsToRemove)
	{
		if (coordsToRemove.empty())
			return grid.CountFilled();

		auto filledMap = BuildFilledIndexMap(grid);

		std::unordered_set<size_t> removedPaletteIndices;
		for (const auto& coord : coordsToRemove)
		{
			if (!grid.IsValidCoord(coord) || !grid.IsFilled(coord))
				continue;

			uint64_t idx = grid.CoordToIndex(coord);
			auto it = filledMap.find(idx);
			if (it != filledMap.end())
				removedPaletteIndices.insert(it->second);

			grid.Clear(coord.x, coord.y, coord.z);
		}

		if (!paletteIndices.empty())
		{
			std::vector<uint8_t> newPalette;
			newPalette.reserve(paletteIndices.size() - removedPaletteIndices.size());
			for (size_t i = 0; i < paletteIndices.size(); i++)
			{
				if (removedPaletteIndices.find(i) == removedPaletteIndices.end())
					newPalette.push_back(paletteIndices[i]);
			}
			paletteIndices = std::move(newPalette);
		}

		return grid.CountFilled();
	}

	std::vector<VoxelFragment> VoxelSplitter::FindConnectedComponents(
		const voxelizer::VoxelGrid& grid,
		const std::vector<uint8_t>& paletteIndices)
	{
		const int sizeX = grid.size.x;
		const int sizeY = grid.size.y;
		const int sizeZ = grid.size.z;

		// Visited tracking
		std::vector<bool> visited(static_cast<size_t>(sizeX) * sizeY * sizeZ, false);
		auto vidx = [&](int x, int y, int z) -> size_t {
			return static_cast<size_t>(x) + static_cast<size_t>(y) * sizeX + static_cast<size_t>(z) * sizeX * sizeY;
		};

		// 6-connected neighbors
		static const glm::ivec3 neighbors[6] = {
			{1, 0, 0}, {-1, 0, 0},
			{0, 1, 0}, {0, -1, 0},
			{0, 0, 1}, {0, 0, -1}
		};

		std::vector<std::vector<glm::ivec3>> components;

		for (int z = 0; z < sizeZ; z++)
		{
			for (int y = 0; y < sizeY; y++)
			{
				for (int x = 0; x < sizeX; x++)
				{
					if (!grid.IsFilled(x, y, z) || visited[vidx(x, y, z)])
						continue;

					// BFS flood fill from this voxel
					std::vector<glm::ivec3> component;
					std::queue<glm::ivec3> queue;
					queue.push({x, y, z});
					visited[vidx(x, y, z)] = true;

					while (!queue.empty())
					{
						glm::ivec3 current = queue.front();
						queue.pop();
						component.push_back(current);

						for (const auto& offset : neighbors)
						{
							glm::ivec3 neighbor = current + offset;
							if (!grid.IsValidCoord(neighbor))
								continue;
							if (visited[vidx(neighbor.x, neighbor.y, neighbor.z)])
								continue;
							if (!grid.IsFilled(neighbor))
								continue;

							visited[vidx(neighbor.x, neighbor.y, neighbor.z)] = true;
							queue.push(neighbor);
						}
					}

					components.push_back(std::move(component));
				}
			}
		}

		// Convert to VoxelFragment
		std::vector<VoxelFragment> fragments;
		fragments.reserve(components.size());

		for (auto& coords : components)
		{
			fragments.push_back(ExtractComponent(grid, paletteIndices, coords));
		}

		return fragments;
	}

	VoxelFragment VoxelSplitter::ExtractComponent(
		const voxelizer::VoxelGrid& sourceGrid,
		const std::vector<uint8_t>& sourcePaletteIndices,
		const std::vector<glm::ivec3>& componentCoords)
	{
		VoxelFragment fragment;

		if (componentCoords.empty())
			return fragment;

		// Find bounding box
		glm::ivec3 minC = componentCoords[0];
		glm::ivec3 maxC = componentCoords[0];
		Vector3 comSum(0.0f);

		for (const auto& coord : componentCoords)
		{
			minC = glm::min(minC, coord);
			maxC = glm::max(maxC, coord);
			comSum += sourceGrid.VoxelCenterToWorld(coord);
		}

		fragment.MinCoord = minC;
		fragment.VoxelCount = componentCoords.size();
		fragment.CenterOfMass = comSum / static_cast<float>(componentCoords.size());

		// Create tight-fitting sub-grid
		glm::ivec3 subSize = maxC - minC + glm::ivec3(1);
		fragment.Grid.size = subSize;
		fragment.Grid.origin = sourceGrid.origin + glm::vec3(minC) * sourceGrid.voxelSize;
		fragment.Grid.voxelSize = sourceGrid.voxelSize;
		fragment.Grid.totalVoxels = static_cast<uint64_t>(subSize.x) * subSize.y * subSize.z;

		// Allocate bit-packed storage
		size_t numWords = (fragment.Grid.totalVoxels + 63) / 64;
		fragment.Grid.data.resize(numWords, 0);

		// Build source filled index map for palette lookup
		auto sourceFilledMap = BuildFilledIndexMap(sourceGrid);

		// Fill sub-grid and extract palette indices
		fragment.PaletteIndices.reserve(componentCoords.size());

		for (const auto& coord : componentCoords)
		{
			glm::ivec3 localCoord = coord - minC;
			fragment.Grid.SetFilled(localCoord);

			// Map palette index from source
			if (!sourcePaletteIndices.empty())
			{
				uint64_t srcIdx = sourceGrid.CoordToIndex(coord);
				auto it = sourceFilledMap.find(srcIdx);
				if (it != sourceFilledMap.end() && it->second < sourcePaletteIndices.size())
					fragment.PaletteIndices.push_back(sourcePaletteIndices[it->second]);
				else
					fragment.PaletteIndices.push_back(0);
			}
		}

		return fragment;
	}
}
