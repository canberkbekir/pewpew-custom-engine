#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace CB
{
	// =========================================================================
	// HeatSourceInfo — describes a single heat emitter in world space
	// =========================================================================
	struct HeatSourceInfo
	{
		glm::vec3 WorldPos;
		float HeatRate;         // heat/sec injection rate
		float MaxTemperature;   // max heat this source can push a cell to
		float Conductivity;     // 0-1
	};

	// =========================================================================
	// HeatOctreeNode — pool-allocated octree node
	// =========================================================================
	struct HeatOctreeNode
	{
		glm::vec3 Center = glm::vec3(0.0f);
		float HalfSize = 0.0f;
		float Heat = 0.0f;
		float MaxHeat = 0.0f;       // subtree max (for query pruning)
		int32_t ChildIndices[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
		int SourceCount = 0;

		bool IsLeaf() const { return ChildIndices[0] == -1; }
	};

	// =========================================================================
	// LeafNeighborEntry — cached face-adjacent neighbor indices for diffusion
	// =========================================================================
	struct LeafNeighborEntry
	{
		int32_t LeafIndex;
		int32_t Neighbors[6] = {-1,-1,-1,-1,-1,-1}; // +X,-X,+Y,-Y,+Z,-Z
	};

	// =========================================================================
	// HeatOctree — world-space octree heat field
	// =========================================================================
	class HeatOctree
	{
	public:
		struct Config
		{
			float MinCellSize = 0.2f;           // minimum half-size (2x voxel size)
			float SubdivideHeatThreshold = 10.0f; // heat gradient triggering split
			float CollapseThreshold = 5.0f;      // max variance for collapse
			float BaseDiffusionRate = 0.3f;
			float BaseDecayRate = 5.0f;
		};

		struct HotCell
		{
			glm::vec3 Center;
			float HalfSize;
			float Heat;
		};

		void Initialize(const Config& config);
		void Clear();
		void Shutdown();

		void InjectHeat(const std::vector<HeatSourceInfo>& sources, float dt);
		void DiffuseHeat(float dt);
		void DecayHeat(float dt);
		void Compact();
		void RebuildNeighborCache();

		float QueryHeat(const glm::vec3& worldPos) const;
		void QueryHotCells(float minHeat, std::vector<HotCell>& outCells) const;

		int GetNodeCount() const { return static_cast<int>(m_Nodes.size()) - static_cast<int>(m_FreeList.size()); }
		int GetLeafCount() const { return static_cast<int>(m_LeafCache.size()); }
		bool IsInitialized() const { return m_Initialized; }

	private:
		int32_t AllocNode();
		void FreeNode(int32_t index);
		int GetOctant(const glm::vec3& nodeCenter, const glm::vec3& point) const;
		void Subdivide(int32_t nodeIndex);
		int32_t FindLeaf(const glm::vec3& worldPos) const;
		int32_t FindOrCreateLeaf(const glm::vec3& worldPos);
		void ExpandRoot(const glm::vec3& point);
		void UpdateMaxHeat(int32_t nodeIndex);
		void QueryHotCellsRecursive(int32_t nodeIndex, float minHeat, std::vector<HotCell>& outCells) const;
		void CollectLeaves(int32_t nodeIndex, std::vector<int32_t>& outLeaves) const;
		bool TryCollapse(int32_t nodeIndex);
		void UpdateMaxHeatRecursive(int32_t nodeIndex);

		std::vector<HeatOctreeNode> m_Nodes;
		std::vector<int32_t> m_FreeList;
		std::vector<LeafNeighborEntry> m_NeighborCache;
		std::vector<int32_t> m_LeafCache;
		Config m_Config;
		int32_t m_RootIndex = -1;
		bool m_Initialized = false;
	};
}
