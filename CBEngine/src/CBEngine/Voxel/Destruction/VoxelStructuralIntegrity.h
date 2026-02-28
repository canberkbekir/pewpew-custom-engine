#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "VoxelDamageMap.h"

#include <Voxelizer.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>

namespace CB
{
	// =========================================================================
	// VoxelStructuralIntegrity
	//
	// Static utility class that determines which voxels are structurally
	// supported (connected to an anchor) and which are floating.
	//
	// Algorithm:
	//   1. Determine anchor voxels (bottom face or manual anchor positions)
	//   2. BFS flood-fill from anchors through filled voxels
	//   3. Any filled voxel NOT reached = unsupported
	//   4. Group unsupported voxels into connected components → collapse clusters
	//
	// Performance:
	//   - Uses a frame budget (max iterations per call) to avoid stalls
	//   - Returns partial results when budget exceeded; caller retries next frame
	// =========================================================================

	struct CollapseCluster
	{
		std::vector<glm::ivec3> Coords; // voxel grid coords in this cluster
		glm::vec3 CenterOfMass;
		uint64_t VoxelCount = 0;
	};

	class VoxelStructuralIntegrity
	{
	public:
		// -----------------------------------------------------------------
		// FindUnsupportedClusters
		//
		// Given a voxel grid and a set of anchor coords, flood-fill to find
		// all grounded voxels. Return connected components of UNgrounded
		// (floating) voxels as CollapseCluster objects.
		//
		// anchorCoords: voxels considered grounded (bottom face, manual, etc.)
		// maxIterations: BFS iteration budget (0 = unlimited)
		//
		// Returns empty vector if everything is supported.
		// -----------------------------------------------------------------
		static std::vector<CollapseCluster> FindUnsupportedClusters(
			const voxelizer::VoxelGrid& grid,
			const std::vector<glm::ivec3>& anchorCoords,
			int maxIterations = 0);

		/// Sparse overload: iterates filledCoords instead of entire grid volume.
		static std::vector<CollapseCluster> FindUnsupportedClusters(
			const voxelizer::VoxelGrid& grid,
			const std::vector<glm::ivec3>& anchorCoords,
			int maxIterations,
			const std::vector<glm::ivec3>& filledCoords);

		// -----------------------------------------------------------------
		// ComputeBottomFaceAnchors
		//
		// Returns all filled voxels at y=0 (the bottom layer of the grid).
		// Used when AnchorMode::BottomFace is set.
		// -----------------------------------------------------------------
		static std::vector<glm::ivec3> ComputeBottomFaceAnchors(
			const voxelizer::VoxelGrid& grid);
	private:
		// 6-connected BFS from a seed set through filled voxels
		static std::unordered_set<glm::ivec3, VoxelCoordHash> FloodFillFromSeeds(
			const voxelizer::VoxelGrid& grid,
			const std::vector<glm::ivec3>& seeds,
			int maxIterations);

		// Group a set of coords into 6-connected components
		static std::vector<CollapseCluster> GroupIntoComponents(
			const std::vector<glm::ivec3>& coords,
			const voxelizer::VoxelGrid& grid);
	};
}