#include "cbpch.h"
#include "HeatOctree.h"

#include <algorithm>
#include <cmath>

namespace CB
{
	// =========================================================================
	// Lifecycle
	// =========================================================================

	void HeatOctree::Initialize(const Config& config)
	{
		m_Config = config;
		m_Nodes.clear();
		m_FreeList.clear();
		m_NeighborCache.clear();
		m_LeafCache.clear();

		// Create root node with a reasonable initial size
		m_RootIndex = AllocNode();
		auto& root = m_Nodes[m_RootIndex];
		root.Center = glm::vec3(0.0f);
		root.HalfSize = 8.0f; // 16x16x16 world units initial coverage
		m_Initialized = true;
	}

	void HeatOctree::Clear()
	{
		if (!m_Initialized) return;

		m_Nodes.clear();
		m_FreeList.clear();
		m_NeighborCache.clear();
		m_LeafCache.clear();

		m_RootIndex = AllocNode();
		auto& root = m_Nodes[m_RootIndex];
		root.Center = glm::vec3(0.0f);
		root.HalfSize = 8.0f;
	}

	void HeatOctree::Shutdown()
	{
		m_Nodes.clear();
		m_Nodes.shrink_to_fit();
		m_FreeList.clear();
		m_FreeList.shrink_to_fit();
		m_NeighborCache.clear();
		m_NeighborCache.shrink_to_fit();
		m_LeafCache.clear();
		m_LeafCache.shrink_to_fit();
		m_RootIndex = -1;
		m_Initialized = false;
	}

	// =========================================================================
	// Pool allocator
	// =========================================================================

	int32_t HeatOctree::AllocNode()
	{
		if (!m_FreeList.empty())
		{
			int32_t idx = m_FreeList.back();
			m_FreeList.pop_back();
			m_Nodes[idx] = HeatOctreeNode{};
			return idx;
		}
		int32_t idx = static_cast<int32_t>(m_Nodes.size());
		m_Nodes.emplace_back();
		return idx;
	}

	void HeatOctree::FreeNode(int32_t index)
	{
		m_Nodes[index] = HeatOctreeNode{};
		m_FreeList.push_back(index);
	}

	// =========================================================================
	// Octant helpers
	// =========================================================================

	int HeatOctree::GetOctant(const glm::vec3& nodeCenter, const glm::vec3& point) const
	{
		int octant = 0;
		if (point.x >= nodeCenter.x) octant |= 1;
		if (point.y >= nodeCenter.y) octant |= 2;
		if (point.z >= nodeCenter.z) octant |= 4;
		return octant;
	}

	static glm::vec3 GetChildCenter(const glm::vec3& parentCenter, float childHalfSize, int octant)
	{
		return parentCenter + glm::vec3(
			(octant & 1) ? childHalfSize : -childHalfSize,
			(octant & 2) ? childHalfSize : -childHalfSize,
			(octant & 4) ? childHalfSize : -childHalfSize
		);
	}

	void HeatOctree::Subdivide(int32_t nodeIndex)
	{
		auto& node = m_Nodes[nodeIndex];
		float childHalf = node.HalfSize * 0.5f;
		float heatShare = node.Heat / 8.0f;

		for (int i = 0; i < 8; i++)
		{
			int32_t childIdx = AllocNode();
			// Re-fetch node ref after AllocNode (vector may reallocate)
			m_Nodes[nodeIndex].ChildIndices[i] = childIdx;
			auto& child = m_Nodes[childIdx];
			child.Center = GetChildCenter(m_Nodes[nodeIndex].Center, childHalf, i);
			child.HalfSize = childHalf;
			child.Heat = heatShare;
			child.MaxHeat = heatShare;
		}
		m_Nodes[nodeIndex].Heat = 0.0f;
	}

	// =========================================================================
	// Dynamic root expansion
	// =========================================================================

	void HeatOctree::ExpandRoot(const glm::vec3& point)
	{
		while (true)
		{
			auto& root = m_Nodes[m_RootIndex];
			glm::vec3 diff = point - root.Center;
			if (std::abs(diff.x) <= root.HalfSize &&
				std::abs(diff.y) <= root.HalfSize &&
				std::abs(diff.z) <= root.HalfSize)
				return; // point is inside root

			// Determine which octant the old root should become in the new larger root
			int oldOctant = 0;
			if (diff.x < 0) oldOctant |= 1; // old root goes in positive direction opposite to point
			if (diff.y < 0) oldOctant |= 2;
			if (diff.z < 0) oldOctant |= 4;

			int32_t newRootIdx = AllocNode();
			auto& newRoot = m_Nodes[newRootIdx];
			float newHalfSize = m_Nodes[m_RootIndex].HalfSize * 2.0f;
			newRoot.HalfSize = newHalfSize;
			newRoot.Center = m_Nodes[m_RootIndex].Center +
				glm::vec3(
					(oldOctant & 1) ? -m_Nodes[m_RootIndex].HalfSize : m_Nodes[m_RootIndex].HalfSize,
					(oldOctant & 2) ? -m_Nodes[m_RootIndex].HalfSize : m_Nodes[m_RootIndex].HalfSize,
					(oldOctant & 4) ? -m_Nodes[m_RootIndex].HalfSize : m_Nodes[m_RootIndex].HalfSize
				);

			// Place old root as the appropriate child
			newRoot.ChildIndices[oldOctant] = m_RootIndex;
			newRoot.MaxHeat = m_Nodes[m_RootIndex].MaxHeat;

			m_RootIndex = newRootIdx;
		}
	}

	// =========================================================================
	// Tree traversal
	// =========================================================================

	int32_t HeatOctree::FindLeaf(const glm::vec3& worldPos) const
	{
		if (m_RootIndex < 0) return -1;

		int32_t current = m_RootIndex;
		while (!m_Nodes[current].IsLeaf())
		{
			int octant = GetOctant(m_Nodes[current].Center, worldPos);
			int32_t child = m_Nodes[current].ChildIndices[octant];
			if (child < 0) return current; // partially subdivided — shouldn't happen, but handle it
			current = child;
		}
		return current;
	}

	int32_t HeatOctree::FindOrCreateLeaf(const glm::vec3& worldPos)
	{
		ExpandRoot(worldPos);

		int32_t current = m_RootIndex;
		while (!m_Nodes[current].IsLeaf())
		{
			int octant = GetOctant(m_Nodes[current].Center, worldPos);
			int32_t child = m_Nodes[current].ChildIndices[octant];
			if (child < 0)
			{
				// Create missing child
				child = AllocNode();
				m_Nodes[current].ChildIndices[octant] = child;
				float childHalf = m_Nodes[current].HalfSize * 0.5f;
				m_Nodes[child].Center = GetChildCenter(m_Nodes[current].Center, childHalf, octant);
				m_Nodes[child].HalfSize = childHalf;
			}
			current = child;
		}
		return current;
	}

	void HeatOctree::UpdateMaxHeat(int32_t nodeIndex)
	{
		auto& node = m_Nodes[nodeIndex];
		if (node.IsLeaf())
		{
			node.MaxHeat = node.Heat;
			return;
		}

		float maxH = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			if (node.ChildIndices[i] >= 0)
				maxH = glm::max(maxH, m_Nodes[node.ChildIndices[i]].MaxHeat);
		}
		node.MaxHeat = maxH;
	}

	// =========================================================================
	// InjectHeat
	// =========================================================================

	void HeatOctree::InjectHeat(const std::vector<HeatSourceInfo>& sources, float dt)
	{
		if (!m_Initialized || sources.empty()) return;

		// Reset source counts
		for (auto& node : m_Nodes)
			node.SourceCount = 0;

		for (const auto& src : sources)
		{
			int32_t leafIdx = FindOrCreateLeaf(src.WorldPos);

			// Subdivide if leaf is too large and we have enough heat
			auto& leaf = m_Nodes[leafIdx];
			if (leaf.HalfSize > m_Config.MinCellSize)
			{
				Subdivide(leafIdx);
				// Re-traverse to the correct child
				leafIdx = FindOrCreateLeaf(src.WorldPos);
			}

			float& leafHeat = m_Nodes[leafIdx].Heat;
			// Per-source cap: a source can only heat a cell up to its MaxTemperature.
			// If the cell is already hotter (from diffusion or a stronger source), skip.
			if (leafHeat < src.MaxTemperature)
				leafHeat = glm::min(leafHeat + src.HeatRate * dt, src.MaxTemperature);
			m_Nodes[leafIdx].SourceCount++;
		}

		// Update MaxHeat bottom-up via recursive traversal from root
		UpdateMaxHeatRecursive(m_RootIndex);
	}

	// =========================================================================
	// DiffuseHeat — runs on neighbor cache
	// =========================================================================

	void HeatOctree::DiffuseHeat(float dt)
	{
		if (m_NeighborCache.empty()) return;

		float diffRate = m_Config.BaseDiffusionRate * dt;

		for (auto& entry : m_NeighborCache)
		{
			auto& leaf = m_Nodes[entry.LeafIndex];
			for (int i = 0; i < 6; i++)
			{
				if (entry.Neighbors[i] < 0) continue;

				auto& neighbor = m_Nodes[entry.Neighbors[i]];
				float delta = leaf.Heat - neighbor.Heat;
				if (delta <= 0.0f) continue; // only flow from hot to cold (avoid double-counting)

				float flow = delta * diffRate;
				flow *= glm::min(leaf.HalfSize, neighbor.HalfSize) / m_Config.MinCellSize;
				flow = glm::min(flow, leaf.Heat); // don't drain more than available
				if (flow <= 0.0f) continue;

				leaf.Heat -= flow;
				neighbor.Heat += flow;
			}
		}
	}

	// =========================================================================
	// DecayHeat
	// =========================================================================

	void HeatOctree::DecayHeat(float dt)
	{
		float decay = m_Config.BaseDecayRate * dt;
		for (auto leafIdx : m_LeafCache)
		{
			auto& leaf = m_Nodes[leafIdx];
			if (leaf.SourceCount > 0) continue; // active sources don't decay
			leaf.Heat = glm::max(0.0f, leaf.Heat - decay);
		}
	}

	// =========================================================================
	// Compact — collapse uniform children, prune dead leaves
	// =========================================================================

	void HeatOctree::Compact()
	{
		if (m_RootIndex < 0) return;
		TryCollapse(m_RootIndex);
	}

	bool HeatOctree::TryCollapse(int32_t nodeIndex)
	{
		auto& node = m_Nodes[nodeIndex];
		if (node.IsLeaf())
			return (node.Heat <= 0.0f && node.SourceCount == 0);

		// Recurse into children first
		bool allChildrenCollapsible = true;
		float heatSum = 0.0f;
		float heatMin = 1e30f;
		float heatMax = 0.0f;
		int childCount = 0;

		for (int i = 0; i < 8; i++)
		{
			if (node.ChildIndices[i] < 0) continue;

			bool childCollapsible = TryCollapse(node.ChildIndices[i]);
			auto& child = m_Nodes[node.ChildIndices[i]];

			if (!child.IsLeaf() || child.SourceCount > 0)
				allChildrenCollapsible = false;

			heatSum += child.Heat;
			heatMin = glm::min(heatMin, child.Heat);
			heatMax = glm::max(heatMax, child.Heat);
			childCount++;
		}

		// Collapse if all children are leaves with low variance and no sources
		if (allChildrenCollapsible && childCount > 0)
		{
			float variance = heatMax - heatMin;
			if (variance < m_Config.CollapseThreshold)
			{
				// Free all children
				for (int i = 0; i < 8; i++)
				{
					if (node.ChildIndices[i] >= 0)
					{
						FreeNode(node.ChildIndices[i]);
						node.ChildIndices[i] = -1;
					}
				}
				node.Heat = heatSum / static_cast<float>(childCount);
				node.MaxHeat = node.Heat;
				return (node.Heat <= 0.0f);
			}
		}

		// Update MaxHeat for this internal node
		UpdateMaxHeat(nodeIndex);
		return false;
	}

	// =========================================================================
	// RebuildNeighborCache
	// =========================================================================

	void HeatOctree::RebuildNeighborCache()
	{
		m_LeafCache.clear();
		m_NeighborCache.clear();
		if (m_RootIndex < 0) return;

		CollectLeaves(m_RootIndex, m_LeafCache);

		static const glm::vec3 offsets[6] = {
			{ 1, 0, 0}, {-1, 0, 0},
			{ 0, 1, 0}, { 0,-1, 0},
			{ 0, 0, 1}, { 0, 0,-1}
		};

		m_NeighborCache.reserve(m_LeafCache.size());
		for (int32_t leafIdx : m_LeafCache)
		{
			LeafNeighborEntry entry;
			entry.LeafIndex = leafIdx;

			const auto& leaf = m_Nodes[leafIdx];
			float probeOffset = leaf.HalfSize + 0.001f;

			for (int i = 0; i < 6; i++)
			{
				glm::vec3 probePos = leaf.Center + offsets[i] * probeOffset;
				int32_t neighborIdx = FindLeaf(probePos);
				if (neighborIdx >= 0 && neighborIdx != leafIdx)
					entry.Neighbors[i] = neighborIdx;
			}

			m_NeighborCache.push_back(entry);
		}
	}

	void HeatOctree::CollectLeaves(int32_t nodeIndex, std::vector<int32_t>& outLeaves) const
	{
		const auto& node = m_Nodes[nodeIndex];
		if (node.IsLeaf())
		{
			outLeaves.push_back(nodeIndex);
			return;
		}

		for (int i = 0; i < 8; i++)
		{
			if (node.ChildIndices[i] >= 0)
				CollectLeaves(node.ChildIndices[i], outLeaves);
		}
	}

	// =========================================================================
	// Queries
	// =========================================================================

	float HeatOctree::QueryHeat(const glm::vec3& worldPos) const
	{
		int32_t leafIdx = FindLeaf(worldPos);
		if (leafIdx < 0) return 0.0f;
		return m_Nodes[leafIdx].Heat;
	}

	void HeatOctree::QueryHotCells(float minHeat, std::vector<HotCell>& outCells) const
	{
		if (m_RootIndex < 0) return;
		QueryHotCellsRecursive(m_RootIndex, minHeat, outCells);
	}

	void HeatOctree::QueryHotCellsRecursive(int32_t nodeIndex, float minHeat, std::vector<HotCell>& outCells) const
	{
		const auto& node = m_Nodes[nodeIndex];

		// Early-exit: if subtree max heat is below threshold, skip entirely
		if (node.MaxHeat < minHeat) return;

		if (node.IsLeaf())
		{
			if (node.Heat >= minHeat)
			{
				outCells.push_back({ node.Center, node.HalfSize, node.Heat });
			}
			return;
		}

		for (int i = 0; i < 8; i++)
		{
			if (node.ChildIndices[i] >= 0)
				QueryHotCellsRecursive(node.ChildIndices[i], minHeat, outCells);
		}
	}

	// =========================================================================
	// Internal helper — recursive MaxHeat update
	// =========================================================================

	void HeatOctree::UpdateMaxHeatRecursive(int32_t nodeIndex)
	{
		auto& node = m_Nodes[nodeIndex];
		if (node.IsLeaf())
		{
			node.MaxHeat = node.Heat;
			return;
		}

		float maxH = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			if (node.ChildIndices[i] >= 0)
			{
				UpdateMaxHeatRecursive(node.ChildIndices[i]);
				maxH = glm::max(maxH, m_Nodes[node.ChildIndices[i]].MaxHeat);
			}
		}
		node.MaxHeat = maxH;
	}
}
