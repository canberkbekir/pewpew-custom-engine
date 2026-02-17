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
		/// Greedy merge filled voxels into larger boxes to reduce shape count.
		/// Returns a list of merged boxes in voxel coordinates.
		static std::vector<MergedBox> GreedyMerge(const voxelizer::VoxelGrid& grid);

		/// Create a Jolt StaticCompoundShape from merged boxes.
		static JPH::RefConst<JPH::Shape> CreateCompoundShape(
			const voxelizer::VoxelGrid& grid,
			const std::vector<MergedBox>& mergedBoxes);

		/// Convenience: merge + create compound shape.
		static JPH::RefConst<JPH::Shape> GenerateFromGrid(const voxelizer::VoxelGrid& grid);
	};
}
