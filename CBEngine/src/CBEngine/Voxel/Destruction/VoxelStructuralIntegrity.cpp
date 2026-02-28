#include "cbpch.h"
#include "VoxelStructuralIntegrity.h"

#include <queue>
#include <algorithm>

namespace CB
{
	// 6-connected neighbor offsets
	static const glm::ivec3 s_Neighbors[6] = {
		{1, 0, 0}, {-1, 0, 0},
		{0, 1, 0}, {0, -1, 0},
		{0, 0, 1}, {0, 0, -1}
	};

	// =========================================================================
	// FloodFillFromSeeds — BFS through filled voxels starting from seed set
	// Returns the set of all voxels reachable (= grounded).
	// =========================================================================
	std::unordered_set<glm::ivec3, VoxelCoordHash> VoxelStructuralIntegrity::FloodFillFromSeeds(
		const voxelizer::VoxelGrid& grid,
		const std::vector<glm::ivec3>& seeds,
		int maxIterations)
	{
		std::unordered_set<glm::ivec3, VoxelCoordHash> visited;
		std::queue<glm::ivec3> frontier;

		for (const auto& s : seeds) {
			if (grid.IsValidCoord(s) && grid.IsFilled(s)) {
				if (visited.insert(s).second)
					frontier.push(s);
			}
		}

		int iterations = 0;
		while (!frontier.empty()) {
			if (maxIterations > 0 && iterations >= maxIterations)
				break; // budget exhausted — caller can retry next frame

			glm::ivec3 cur = frontier.front();
			frontier.pop();
			++iterations;

			for (const auto& offset : s_Neighbors) {
				glm::ivec3 nb = cur + offset;
				if (!grid.IsValidCoord(nb))
					continue;
				if (!grid.IsFilled(nb))
					continue;
				if (visited.insert(nb).second)
					frontier.push(nb);
			}
		}

		return visited;
	}

	// =========================================================================
	// ComputeBottomFaceAnchors — all filled voxels at y=0
	// =========================================================================
	std::vector<glm::ivec3> VoxelStructuralIntegrity::ComputeBottomFaceAnchors(
		const voxelizer::VoxelGrid& grid)
	{
		std::vector<glm::ivec3> anchors;
		int w = grid.size.x;
		int d = grid.size.z;

		for (int x = 0; x < w; ++x)
			for (int z = 0; z < d; ++z) {
				if (grid.IsFilled(x, 0, z))
					anchors.emplace_back(x, 0, z);
			}

		return anchors;
	}

	// =========================================================================
	// GroupIntoComponents — group a set of coords into 6-connected clusters
	// =========================================================================
	std::vector<CollapseCluster> VoxelStructuralIntegrity::GroupIntoComponents(
		const std::vector<glm::ivec3>& coords,
		const voxelizer::VoxelGrid& grid)
	{
		std::vector<CollapseCluster> clusters;
		if (coords.empty())
			return clusters;

		// Build lookup set for fast membership test
		std::unordered_set<glm::ivec3, VoxelCoordHash> remaining(coords.begin(), coords.end());

		while (!remaining.empty()) {
			// BFS from an arbitrary starting point
			CollapseCluster cluster;
			std::queue<glm::ivec3> frontier;

			auto startIt = remaining.begin();
			glm::ivec3 start = *startIt;
			remaining.erase(startIt);
			frontier.push(start);
			cluster.Coords.push_back(start);

			while (!frontier.empty()) {
				glm::ivec3 cur = frontier.front();
				frontier.pop();

				for (const auto& offset : s_Neighbors) {
					glm::ivec3 nb = cur + offset;
					auto it = remaining.find(nb);
					if (it != remaining.end()) {
						remaining.erase(it);
						frontier.push(nb);
						cluster.Coords.push_back(nb);
					}
				}
			}

			// Compute center of mass (average of voxel world-space centers)
			cluster.VoxelCount = cluster.Coords.size();
			glm::vec3 sum(0.0f);
			for (const auto& c : cluster.Coords)
				sum += grid.VoxelCenterToWorld(c);
			cluster.CenterOfMass = Vector3(sum.x, sum.y, sum.z) / static_cast<float>(cluster.VoxelCount);

			clusters.push_back(std::move(cluster));
		}

		return clusters;
	}

	// =========================================================================
	// FindUnsupportedClusters — main entry point
	// =========================================================================
	std::vector<CollapseCluster> VoxelStructuralIntegrity::FindUnsupportedClusters(
		const voxelizer::VoxelGrid& grid,
		const std::vector<glm::ivec3>& anchorCoords,
		int maxIterations)
	{
		if (anchorCoords.empty()) {
			// No anchors = everything is unsupported → return all filled as one or more clusters
			auto allFilled = grid.GetFilledCoords();
			return GroupIntoComponents(allFilled, grid);
		}

		// 1. Flood-fill from anchors to find all grounded voxels
		auto grounded = FloodFillFromSeeds(grid, anchorCoords, maxIterations);

		// 2. Find all filled voxels NOT in the grounded set = unsupported
		//    Iterate grid directly instead of calling GetFilledCoords() which
		//    allocates a large intermediate vector and scans all voxels twice.
		std::vector<glm::ivec3> unsupported;
		for (int z = 0; z < grid.size.z; ++z)
			for (int y = 0; y < grid.size.y; ++y)
				for (int x = 0; x < grid.size.x; ++x) {
					if (grid.IsFilled(x, y, z) &&
						grounded.find({x, y, z}) == grounded.end())
						unsupported.push_back({x, y, z});
				}

		if (unsupported.empty())
			return {}; // everything is supported

		// 3. Group unsupported voxels into connected components
		return GroupIntoComponents(unsupported, grid);
	}

	// =========================================================================
	// FindUnsupportedClusters — sparse overload using filledCoords
	// Replaces O(W*H*D) triple loop with O(filledCoords.size()) iteration
	// =========================================================================
	std::vector<CollapseCluster> VoxelStructuralIntegrity::FindUnsupportedClusters(
		const voxelizer::VoxelGrid& grid,
		const std::vector<glm::ivec3>& anchorCoords,
		int maxIterations,
		const std::vector<glm::ivec3>& filledCoords)
	{
		if (anchorCoords.empty()) {
			// No anchors = everything is unsupported
			return GroupIntoComponents(filledCoords, grid);
		}

		// 1. Flood-fill from anchors to find all grounded voxels
		auto grounded = FloodFillFromSeeds(grid, anchorCoords, maxIterations);

		// 2. Find all filled voxels NOT in the grounded set — iterate sparse list only
		std::vector<glm::ivec3> unsupported;
		for (const auto& coord : filledCoords) {
			if (grounded.find(coord) == grounded.end())
				unsupported.push_back(coord);
		}

		if (unsupported.empty())
			return {};

		// 3. Group unsupported voxels into connected components
		return GroupIntoComponents(unsupported, grid);
	}
}