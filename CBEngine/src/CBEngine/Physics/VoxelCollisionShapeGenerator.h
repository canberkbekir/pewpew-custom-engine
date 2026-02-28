#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <Voxelizer.h>
#include <vector>

namespace CB
{
	struct MergedBox
	{
		glm::ivec3 Min;
		glm::ivec3 Max;
	};

	class VoxelCollisionShapeGenerator
	{
	public:
		/// Threshold: fragments with this many voxels or fewer use CreateSimpleShape
		static constexpr int kSmallFragmentThreshold = 5;

		/// Greedy merge filled voxels into larger boxes to reduce shape count.
		/// Returns a list of merged boxes in voxel coordinates.
		static std::vector<MergedBox> GreedyMerge(const voxelizer::VoxelGrid& grid);

		/// Sparse overload: iterates only filledCoords instead of entire grid volume.
		static std::vector<MergedBox> GreedyMerge(const voxelizer::VoxelGrid& grid,
		                                           const std::vector<glm::ivec3>& filledCoords);

		/// Create a Jolt StaticCompoundShape from merged boxes.
		static JPH::RefConst<JPH::Shape> CreateCompoundShape(
			const voxelizer::VoxelGrid& grid,
			const std::vector<MergedBox>& mergedBoxes);

		/// Convenience: merge + create compound shape.
		static JPH::RefConst<JPH::Shape> GenerateFromGrid(const voxelizer::VoxelGrid& grid);

		/// AABB-based shape for small fragments — O(filled) instead of full GreedyMerge.
		/// Computes the axis-aligned bounding box of all filled voxels and creates a
		/// single BoxShape in a StaticCompoundShape.
		static JPH::RefConst<JPH::Shape> CreateSimpleShape(const voxelizer::VoxelGrid& grid);
	};
}