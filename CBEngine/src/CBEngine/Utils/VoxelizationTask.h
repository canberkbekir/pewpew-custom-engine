#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Voxel/VoxelizerAPI.h"
#include "VoxelPalette.h"

#include <future>
#include <atomic>
#include <vector>

namespace CB
{
	enum class VoxelTaskStatus
	{
		Idle,
		Running,
		Completed,
		Failed,
		Cancelled
	};

	struct VoxelTaskResult
	{
		voxelizer::VoxelGrid Grid;
		std::vector<Vector3> VoxelColors;
		std::vector<Vector2> VoxelUVs;
		uint64_t VoxelCount = 0;
		bool HasColors = false;
		bool HasUVs = false;

		VoxelPalette Palette;
		std::vector<uint8_t> PaletteIndices;
		bool HasPalette = false;
	};

	class VoxelizationTask
	{
	public:
		VoxelizationTask() = default;
		~VoxelizationTask();

		// Start voxelization from a mesh file
		void Start(const String& meshPath,const VoxelizeSettings& settings);

		// Start voxelization from a mesh file with texture coloring
		void StartWithColors(const String& meshPath,const String& texturePath,const VoxelizeSettings& settings);

		// Start voxelization from CPU vertex/index data
		void StartFromData(const std::vector<Vertex>& vertices,const std::vector<uint32_t>& indices,
		                   const VoxelizeSettings& settings);

		// Start voxelization from CPU data with texture coloring
		void StartFromDataWithColors(const std::vector<Vertex>& vertices,const std::vector<uint32_t>& indices,
		                             const String& texturePath,const VoxelizeSettings& settings);

		// Poll status
		bool IsComplete() const { return m_Status == VoxelTaskStatus::Completed; }
		bool IsFailed() const { return m_Status == VoxelTaskStatus::Failed; }
		bool IsRunning() const { return m_Status == VoxelTaskStatus::Running; }
		bool IsIdle() const { return m_Status == VoxelTaskStatus::Idle; }
		VoxelTaskStatus GetStatus() const { return m_Status; }

		// Get result (only valid when IsComplete())
		const VoxelTaskResult& GetResult() const { return m_Result; }

		// Create mesh from result (main thread only - OpenGL calls)
		Ref<Mesh> CreateMeshFromResult();
		Ref<Mesh> CreateColoredMeshFromResult();
		Ref<Mesh> CreatePaletteMeshFromResult();

		// Cancel a running task
		void Cancel();
	private:
		void WaitForCompletion();

		std::atomic<VoxelTaskStatus> m_Status{VoxelTaskStatus::Idle};
		std::atomic<bool> m_CancelRequested{false};
		std::future<void> m_Future;
		VoxelTaskResult m_Result;
	};
}