#include "cbpch.h"
#include "VoxelCollisionShapeGenerator.h"
#include "CBEngine/Voxel/Destruction/VoxelTintTypes.h" // VoxelCoordHash

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

namespace CB
{
	std::vector<MergedBox> VoxelCollisionShapeGenerator::GreedyMerge(const voxelizer::VoxelGrid& grid)
	{
		std::vector<MergedBox> result;

		const int sizeX = grid.size.x;
		const int sizeY = grid.size.y;
		const int sizeZ = grid.size.z;

		// Track which voxels have been merged
		std::vector<bool> visited(sizeX * sizeY * sizeZ, false);

		auto idx = [&](int x,int y,int z) -> size_t
		{
			return static_cast<size_t>(x) + static_cast<size_t>(y) * sizeX + static_cast<size_t>(z) * sizeX * sizeY;
		};

		auto isFilledAndFree = [&](int x,int y,int z) -> bool
		{
			if (x < 0 || x >= sizeX || y < 0 || y >= sizeY || z < 0 || z >= sizeZ)
				return false;
			return grid.IsFilled(x, y, z) && !visited[idx(x, y, z)];
		};

		for (int z = 0; z < sizeZ; z++) {
			for (int y = 0; y < sizeY; y++) {
				for (int x = 0; x < sizeX; x++) {
					if (!isFilledAndFree(x, y, z))
						continue;

					// Start a new box at (x, y, z)
					int maxX = x, maxY = y, maxZ = z;

					// Extend along X
					while (maxX + 1 < sizeX && isFilledAndFree(maxX + 1, y, z))
						maxX++;

					// Extend along Y - check full X row
					bool canExtendY = true;
					while (canExtendY && maxY + 1 < sizeY) {
						for (int ix = x; ix <= maxX; ix++) {
							if (!isFilledAndFree(ix, maxY + 1, z)) {
								canExtendY = false;
								break;
							}
						}
						if (canExtendY)
							maxY++;
					}

					// Extend along Z - check full XY plane
					bool canExtendZ = true;
					while (canExtendZ && maxZ + 1 < sizeZ) {
						for (int iy = y; iy <= maxY; iy++) {
							for (int ix = x; ix <= maxX; ix++) {
								if (!isFilledAndFree(ix, iy, maxZ + 1)) {
									canExtendZ = false;
									break;
								}
							}
							if (!canExtendZ)
								break;
						}
						if (canExtendZ)
							maxZ++;
					}

					// Mark all voxels in this box as visited
					for (int iz = z; iz <= maxZ; iz++)
						for (int iy = y; iy <= maxY; iy++)
							for (int ix = x; ix <= maxX; ix++)
								visited[idx(ix, iy, iz)] = true;

					result.push_back({{x, y, z}, {maxX, maxY, maxZ}});
				}
			}
		}

		return result;
	}

	// =========================================================================
	// GreedyMerge — sparse overload using filledCoords
	// Iterates only filled voxels as starting points instead of entire volume
	// =========================================================================
	std::vector<MergedBox> VoxelCollisionShapeGenerator::GreedyMerge(
		const voxelizer::VoxelGrid& grid,
		const std::vector<glm::ivec3>& filledCoords)
	{
		std::vector<MergedBox> result;

		const int sizeX = grid.size.x;
		const int sizeY = grid.size.y;
		const int sizeZ = grid.size.z;

		// Build a set for O(1) filled+unvisited checks
		std::unordered_set<glm::ivec3, VoxelCoordHash> remaining(filledCoords.begin(), filledCoords.end());

		// Sort filledCoords for deterministic merge order (z, y, x)
		std::vector<glm::ivec3> sorted = filledCoords;
		std::sort(sorted.begin(), sorted.end(), [](const glm::ivec3& a, const glm::ivec3& b) {
			if (a.z != b.z) return a.z < b.z;
			if (a.y != b.y) return a.y < b.y;
			return a.x < b.x;
		});

		for (const auto& start : sorted) {
			if (remaining.find(start) == remaining.end())
				continue; // already merged

			int x = start.x, y = start.y, z = start.z;
			int maxX = x, maxY = y, maxZ = z;

			// Extend along X
			while (maxX + 1 < sizeX && remaining.count({maxX + 1, y, z}))
				maxX++;

			// Extend along Y — check full X row
			bool canExtendY = true;
			while (canExtendY && maxY + 1 < sizeY) {
				for (int ix = x; ix <= maxX; ix++) {
					if (!remaining.count({ix, maxY + 1, z})) {
						canExtendY = false;
						break;
					}
				}
				if (canExtendY)
					maxY++;
			}

			// Extend along Z — check full XY plane
			bool canExtendZ = true;
			while (canExtendZ && maxZ + 1 < sizeZ) {
				for (int iy = y; iy <= maxY; iy++) {
					for (int ix = x; ix <= maxX; ix++) {
						if (!remaining.count({ix, iy, maxZ + 1})) {
							canExtendZ = false;
							break;
						}
					}
					if (!canExtendZ)
						break;
				}
				if (canExtendZ)
					maxZ++;
			}

			// Mark all voxels in this box as visited
			for (int iz = z; iz <= maxZ; iz++)
				for (int iy = y; iy <= maxY; iy++)
					for (int ix = x; ix <= maxX; ix++)
						remaining.erase({ix, iy, iz});

			result.push_back({{x, y, z}, {maxX, maxY, maxZ}});
		}

		return result;
	}

	JPH::RefConst<JPH::Shape> VoxelCollisionShapeGenerator::CreateCompoundShape(
		const voxelizer::VoxelGrid& grid,
		const std::vector<MergedBox>& mergedBoxes)
	{
		if (mergedBoxes.empty())
			return nullptr;

		// Build compound shape (even for single box, to preserve correct position)
		JPH::StaticCompoundShapeSettings compoundSettings;

		for (const auto& box : mergedBoxes) {
			glm::vec3 halfExtents(
				(box.Max.x - box.Min.x + 1) * 0.5f * grid.voxelSize.x,
				(box.Max.y - box.Min.y + 1) * 0.5f * grid.voxelSize.y,
				(box.Max.z - box.Min.z + 1) * 0.5f * grid.voxelSize.z);

			// Position in mesh local space (matches mesh vertex positions)
			glm::vec3 center = grid.origin + glm::vec3(
				(box.Min.x + box.Max.x + 1) * 0.5f * grid.voxelSize.x,
				(box.Min.y + box.Max.y + 1) * 0.5f * grid.voxelSize.y,
				(box.Min.z + box.Max.z + 1) * 0.5f * grid.voxelSize.z);

			// Use a convex radius that fits within the smallest half-extent
			// (Jolt's default 0.05 can exceed small voxel boxes, making them degenerate)
			float minHalf = std::min({halfExtents.x, halfExtents.y, halfExtents.z});
			float convexRadius = std::min(minHalf * 0.5f, JPH::cDefaultConvexRadius);

			compoundSettings.AddShape(
				JPH::Vec3(center.x, center.y, center.z),
				JPH::Quat::sIdentity(),
				new JPH::BoxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z), convexRadius));
		}

		auto result = compoundSettings.Create();
		if (result.HasError()) {
			CB_CORE_ERROR("Failed to create compound shape: {0}", result.GetError().c_str());
			return nullptr;
		}

		return result.Get();
	}

	JPH::RefConst<JPH::Shape> VoxelCollisionShapeGenerator::GenerateFromGrid(const voxelizer::VoxelGrid& grid)
	{
		auto mergedBoxes = GreedyMerge(grid);
		CB_CORE_INFO("VoxelCollisionShapeGenerator: {0} voxels merged into {1} boxes",
		             grid.CountFilled(), mergedBoxes.size());
		return CreateCompoundShape(grid, mergedBoxes);
	}

	JPH::RefConst<JPH::Shape> VoxelCollisionShapeGenerator::CreateSimpleShape(const voxelizer::VoxelGrid& grid)
	{
		// Compute AABB of all filled voxels in one pass
		glm::ivec3 bmin(INT_MAX), bmax(INT_MIN);
		bool found = false;

		for (uint64_t i = 0; i < grid.totalVoxels; ++i) {
			if (!grid.IsFilled(i)) continue;
			glm::ivec3 c = grid.IndexToCoord(i);
			bmin = glm::min(bmin, c);
			bmax = glm::max(bmax, c);
			found = true;
		}

		if (!found)
			return nullptr;

		// Create a single box shape matching the AABB
		std::vector<MergedBox> singleBox = { {bmin, bmax} };
		return CreateCompoundShape(grid, singleBox);
	}
}