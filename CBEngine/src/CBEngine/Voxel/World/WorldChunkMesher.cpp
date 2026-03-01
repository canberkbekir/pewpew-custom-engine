#include "cbpch.h"
#include "WorldChunkMesher.h"
#include "WorldGrid.h"

namespace CB
{
	// Face directions: +X, -X, +Y, -Y, +Z, -Z
	struct FaceDir
	{
		glm::ivec3 Normal;     // face normal direction
		int Axis;              // which axis is the slice (0=X, 1=Y, 2=Z)
		int U, V;              // the two axes forming the face plane
		bool Positive;         // true if facing positive direction
	};

	static constexpr FaceDir s_FaceDirs[6] = {
		{ { 1, 0, 0}, 0, 2, 1, true  }, // +X: slice along X, face plane is ZY
		{ {-1, 0, 0}, 0, 2, 1, false }, // -X
		{ { 0, 1, 0}, 1, 0, 2, true  }, // +Y: slice along Y, face plane is XZ
		{ { 0,-1, 0}, 1, 0, 2, false }, // -Y
		{ { 0, 0, 1}, 2, 0, 1, true  }, // +Z: slice along Z, face plane is XY
		{ { 0, 0,-1}, 2, 0, 1, false }, // -Z
	};

	// Check if a cell is solid (non-air) at a given world voxel coordinate
	static bool IsSolid(const WorldGrid& grid, glm::ivec3 worldVoxel)
	{
		return !grid.GetCell(worldVoxel).IsAir();
	}

	// Check if a supervoxel (step^3 block) is solid — sample center cell
	static bool IsSolidLOD(const WorldGrid& grid, glm::ivec3 worldVoxel, int step)
	{
		if (step <= 1) return IsSolid(grid, worldVoxel);
		// Sample at center of the supervoxel block
		glm::ivec3 center = worldVoxel + glm::ivec3(step / 2);
		if (!grid.GetCell(center).IsAir()) return true;
		// Fallback: sample corner
		return !grid.GetCell(worldVoxel).IsAir();
	}

	// Compute ambient occlusion value (0-3) for a vertex corner on a face
	static int ComputeAO(bool side1, bool side2, bool corner)
	{
		if (side1 && side2)
			return 0;
		return 3 - (static_cast<int>(side1) + static_cast<int>(side2) + static_cast<int>(corner));
	}

	// Convert AO level (0-3) to a multiplier (0.2 - 1.0)
	static float AOToFloat(int ao)
	{
		static constexpr float aoValues[4] = { 0.2f, 0.5f, 0.75f, 1.0f };
		return aoValues[ao];
	}

	// Get world voxel coordinate from chunk coord + local position
	static glm::ivec3 ToWorldVoxel(const glm::ivec3& chunkCoord, int x, int y, int z)
	{
		return chunkCoord * CHUNK_SIZE + glm::ivec3(x, y, z);
	}

	// Sample palette index for a supervoxel. For step=1 this is trivial.
	// For LOD, sample (x*step, y*step, z*step); fallback to center.
	static uint8_t SamplePaletteIndex(const WorldChunk& chunk, const WorldGrid& grid,
	                                   int localX, int localY, int localZ, int step)
	{
		if (step <= 1) {
			return chunk.At(localX, localY, localZ).PaletteIndex;
		}

		// Sample at the base corner
		const WorldCell& cell = grid.GetCell(ToWorldVoxel(chunk.Coord, localX, localY, localZ));
		if (!cell.IsAir()) return cell.PaletteIndex;

		// Fallback: center of supervoxel
		int cx = localX + step / 2, cy = localY + step / 2, cz = localZ + step / 2;
		const WorldCell& center = grid.GetCell(ToWorldVoxel(chunk.Coord, cx, cy, cz));
		return center.PaletteIndex;
	}

	Ref<Mesh> WorldChunkMesher::BuildMesh(WorldChunk& chunk, const WorldGrid& grid, int step)
	{
		// Ensure chunk is decompressed before meshing
		chunk.EnsureDecompressed();

		int gridSize = CHUNK_SIZE / step; // 32, 16, or 8

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		vertices.reserve(1024);
		indices.reserve(2048);

		bool skipAO = (step >= 4); // Skip AO for quarter-res

		for (int faceIdx = 0; faceIdx < 6; faceIdx++) {
			const FaceDir& fd = s_FaceDirs[faceIdx];
			glm::vec3 normal(fd.Normal);

			// For each slice along the face axis
			for (int d = 0; d < gridSize; d++) {
				// Build mask of exposed faces for this slice
				struct FaceInfo {
					uint8_t PaletteIndex;
					bool Exposed;
				};
				// Max grid size is 32
				FaceInfo mask[32][32]; // [v][u]
				std::memset(mask, 0, sizeof(mask));

				for (int v = 0; v < gridSize; v++) {
					for (int u = 0; u < gridSize; u++) {
						// Reconstruct 3D local coord from (d, u, v) based on face axis
						glm::ivec3 local;
						local[fd.Axis] = d * step;
						local[fd.U] = u * step;
						local[fd.V] = v * step;

						glm::ivec3 worldVoxel = ToWorldVoxel(chunk.Coord, local.x, local.y, local.z);

						// Check if supervoxel is solid
						if (!IsSolidLOD(grid, worldVoxel, step))
							continue;

						// Check neighbor in face direction (at supervoxel scale)
						glm::ivec3 neighbor = worldVoxel + fd.Normal * step;
						if (!IsSolidLOD(grid, neighbor, step)) {
							mask[v][u].Exposed = true;
							mask[v][u].PaletteIndex = SamplePaletteIndex(chunk, grid, local.x, local.y, local.z, step);
						}
					}
				}

				// Greedy merge: scan the 2D mask
				bool visited[32][32];
				std::memset(visited, 0, sizeof(visited));

				for (int v = 0; v < gridSize; v++) {
					for (int u = 0; u < gridSize; u++) {
						if (!mask[v][u].Exposed || visited[v][u])
							continue;

						uint8_t palIdx = mask[v][u].PaletteIndex;

						// Expand along U
						int width = 1;
						while (u + width < gridSize &&
							   mask[v][u + width].Exposed &&
							   !visited[v][u + width] &&
							   mask[v][u + width].PaletteIndex == palIdx)
							width++;

						// Expand along V
						int height = 1;
						bool done = false;
						while (v + height < gridSize && !done) {
							for (int k = 0; k < width; k++) {
								if (!mask[v + height][u + k].Exposed ||
									visited[v + height][u + k] ||
									mask[v + height][u + k].PaletteIndex != palIdx) {
									done = true;
									break;
								}
							}
							if (!done) height++;
						}

						// Mark visited
						for (int dv = 0; dv < height; dv++)
							for (int du = 0; du < width; du++)
								visited[v + dv][u + du] = true;

						// Emit quad
						float faceOffset = fd.Positive ? static_cast<float>(step) : 0.0f;
						float voxelStep = static_cast<float>(step);

						glm::vec3 corners[4];
						for (int ci = 0; ci < 4; ci++) {
							float cu = (ci == 1 || ci == 2) ? static_cast<float>(u + width) * voxelStep : static_cast<float>(u) * voxelStep;
							float cv = (ci == 2 || ci == 3) ? static_cast<float>(v + height) * voxelStep : static_cast<float>(v) * voxelStep;

							glm::vec3 pos;
							pos[fd.Axis] = (static_cast<float>(d) * voxelStep + faceOffset);
							pos[fd.U] = cu;
							pos[fd.V] = cv;

							// Convert to world space
							pos = (glm::vec3(chunk.Coord * CHUNK_SIZE) + pos) * grid.VoxelSize;
							corners[ci] = pos;
						}

						// Compute AO for each corner vertex
						int aoValues[4] = { 3, 3, 3, 3 };
						if (!skipAO) {
							for (int ci = 0; ci < 4; ci++) {
								int cornerU = (ci == 1 || ci == 2) ? u + width - 1 : u;
								int cornerV = (ci == 2 || ci == 3) ? v + height - 1 : v;

								glm::ivec3 local;
								local[fd.Axis] = d * step;
								local[fd.U] = cornerU * step;
								local[fd.V] = cornerV * step;
								glm::ivec3 wv = ToWorldVoxel(chunk.Coord, local.x, local.y, local.z);

								int duSign = (ci == 1 || ci == 2) ? 1 : -1;
								int dvSign = (ci == 2 || ci == 3) ? 1 : -1;

								glm::ivec3 sideU = wv + fd.Normal * step;
								sideU[fd.U] += duSign * step;

								glm::ivec3 sideV = wv + fd.Normal * step;
								sideV[fd.V] += dvSign * step;

								glm::ivec3 diag = wv + fd.Normal * step;
								diag[fd.U] += duSign * step;
								diag[fd.V] += dvSign * step;

								aoValues[ci] = ComputeAO(
									IsSolidLOD(grid, sideU, step),
									IsSolidLOD(grid, sideV, step),
									IsSolidLOD(grid, diag, step)
								);
							}
						}

						// Add 4 vertices
						uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
						for (int ci = 0; ci < 4; ci++) {
							Vertex vert;
							vert.Position = corners[ci];
							vert.Normal = normal;
							vert.TexCoords = { 0.0f, 0.0f };
							vert.Tangent = { 0.0f, 0.0f, 0.0f };
							vert.Bitangent = { 0.0f, 0.0f, 0.0f };
							float ao = AOToFloat(aoValues[ci]);
							vert.Color = Vector3(ao);
							vert.PaletteIndex = static_cast<float>(palIdx);
							vertices.push_back(vert);
						}

						// Two triangles per quad — flip based on AO
						if (aoValues[0] + aoValues[2] > aoValues[1] + aoValues[3]) {
							indices.push_back(baseIdx + 0);
							indices.push_back(baseIdx + 1);
							indices.push_back(baseIdx + 2);
							indices.push_back(baseIdx + 0);
							indices.push_back(baseIdx + 2);
							indices.push_back(baseIdx + 3);
						} else {
							indices.push_back(baseIdx + 1);
							indices.push_back(baseIdx + 2);
							indices.push_back(baseIdx + 3);
							indices.push_back(baseIdx + 1);
							indices.push_back(baseIdx + 3);
							indices.push_back(baseIdx + 0);
						}
					}
				}
			}
		}

		if (vertices.empty())
			return nullptr;

		return CreateRef<Mesh>(vertices, indices, false);
	}
}
