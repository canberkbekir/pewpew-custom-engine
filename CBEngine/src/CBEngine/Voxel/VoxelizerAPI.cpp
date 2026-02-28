#include "cbpch.h"
#include "VoxelizerAPI.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Debug/Instrumentor.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

#include <stb_image.h>
#include <TriBox.h>

namespace CB
{
	bool VoxelizerAPI::s_Initialized = false;
	Ref<Shader> VoxelizerAPI::s_VoxelShader = nullptr;
	std::vector<Vertex> VoxelizerAPI::s_UnitCubeVertices;
	std::vector<uint32_t> VoxelizerAPI::s_UnitCubeIndices;

	// Face directions for neighbor-based culling (matches CreateUnitCube face order)
	// Face 0: Z+ front, Face 1: Z- back, Face 2: Y+ top, Face 3: Y- bottom, Face 4: X+ right, Face 5: X- left
	static const glm::ivec3 s_FaceDirections[6] = {
		{0, 0, 1}, // Face 0: Z+ front
		{0, 0, -1}, // Face 1: Z- back
		{0, 1, 0}, // Face 2: Y+ top
		{0, -1, 0}, // Face 3: Y- bottom
		{1, 0, 0}, // Face 4: X+ right
		{-1, 0, 0}, // Face 5: X- left
	};

	// Check if a face should be emitted (neighbor is empty or out of bounds)
	static bool ShouldEmitFace(const voxelizer::VoxelGrid& grid,const glm::ivec3& coord,int faceIdx)
	{
		glm::ivec3 neighbor = coord + s_FaceDirections[faceIdx];
		if (!grid.IsValidCoord(neighbor))
			return true; // Boundary voxel — always emit outward-facing face
		return !grid.IsFilled(neighbor);
	}

	// ============================================================================
	// TextureSampler Implementation
	// ============================================================================

	TextureSampler::~TextureSampler()
	{
		if (m_Data) {
			stbi_image_free(m_Data);
			m_Data = nullptr;
		}
	}

	bool TextureSampler::Load(const String& filePath)
	{
		if (m_Data) {
			stbi_image_free(m_Data);
			m_Data = nullptr;
		}

		stbi_set_flip_vertically_on_load(1);
		int width, height, channels;
		m_Data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

		if (!m_Data) {
			CB_CORE_ERROR("TextureSampler: Failed to load texture: {0}", filePath);
			return false;
		}

		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);
		m_Channels = channels;

		CB_CORE_INFO("TextureSampler: Loaded {0} ({1}x{2}, {3} channels)",
		             filePath, m_Width, m_Height, m_Channels);
		return true;
	}

	Vector3 TextureSampler::Sample(const Vector2& uv) const
	{
		if (!m_Data)
			return Vector3(1.0f);

		// Wrap UV coordinates
		float u = uv.x - std::floor(uv.x);
		float v = uv.y - std::floor(uv.y);

		uint32_t x = static_cast<uint32_t>(u * (m_Width - 1));
		uint32_t y = static_cast<uint32_t>(v * (m_Height - 1));

		x = std::min(x, m_Width - 1);
		y = std::min(y, m_Height - 1);

		size_t idx = (y * m_Width + x) * m_Channels;

		float r = m_Data[idx] / 255.0f;
		float g = (m_Channels > 1) ? m_Data[idx + 1] / 255.0f : r;
		float b = (m_Channels > 2) ? m_Data[idx + 2] / 255.0f : r;

		return Vector3(r, g, b);
	}

	Vector3 TextureSampler::SampleBilinear(const Vector2& uv) const
	{
		if (!m_Data)
			return Vector3(1.0f);

		// Wrap UV coordinates
		float u = uv.x - std::floor(uv.x);
		float v = uv.y - std::floor(uv.y);

		float fx = u * (m_Width - 1);
		float fy = v * (m_Height - 1);

		uint32_t x0 = static_cast<uint32_t>(fx);
		uint32_t y0 = static_cast<uint32_t>(fy);
		uint32_t x1 = std::min(x0 + 1, m_Width - 1);
		uint32_t y1 = std::min(y0 + 1, m_Height - 1);

		float sx = fx - x0;
		float sy = fy - y0;

		auto getPixel = [this](uint32_t x,uint32_t y) -> Vector3
		{
			size_t idx = (y * m_Width + x) * m_Channels;
			float r = m_Data[idx] / 255.0f;
			float g = (m_Channels > 1) ? m_Data[idx + 1] / 255.0f : r;
			float b = (m_Channels > 2) ? m_Data[idx + 2] / 255.0f : r;
			return Vector3(r, g, b);
		};

		Vector3 c00 = getPixel(x0, y0);
		Vector3 c10 = getPixel(x1, y0);
		Vector3 c01 = getPixel(x0, y1);
		Vector3 c11 = getPixel(x1, y1);

		Vector3 c0 = mix(c00, c10, sx);
		Vector3 c1 = mix(c01, c11, sx);

		return mix(c0, c1, sy);
	}

	// ============================================================================
	// VoxelizerAPI Implementation
	// ============================================================================

	void VoxelizerAPI::Init()
	{
		CB_PROFILE_FUNCTION();

		if (s_Initialized)
			return;

		// Create unit cube geometry (centered at origin, size 1x1x1)
		CreateUnitCube(s_UnitCubeVertices, s_UnitCubeIndices);

		s_Initialized = true;
		CB_CORE_INFO("VoxelizerAPI initialized");
	}

	void VoxelizerAPI::Shutdown()
	{
		s_VoxelShader = nullptr;
		s_UnitCubeVertices.clear();
		s_UnitCubeIndices.clear();
		s_Initialized = false;
	}

	void VoxelizerAPI::CreateUnitCube(std::vector<Vertex>& vertices,std::vector<uint32_t>& indices)
	{
		vertices.clear();
		indices.clear();

		// Unit cube centered at origin (-0.5 to 0.5)
		// Each face has its own vertices for proper normals

		// Front face (Z+)
		vertices.push_back({
			{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});

		// Back face (Z-)
		vertices.push_back({
			{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});

		// Top face (Y+)
		vertices.push_back({
			{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f}
		});

		// Bottom face (Y-)
		vertices.push_back({
			{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f},
			{1.0f, 1.0f, 1.0f}
		});

		// Right face (X+)
		vertices.push_back({
			{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});

		// Left face (X-)
		vertices.push_back({
			{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});
		vertices.push_back({
			{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		});

		// Indices for 6 faces (2 triangles per face)
		for (uint32_t face = 0; face < 6; ++face) {
			uint32_t base = face * 4;
			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 2);
			indices.push_back(base + 3);
			indices.push_back(base + 0);
		}
	}

	VoxelMeshData VoxelizerAPI::Voxelize(const Ref<Mesh>& mesh,
	                                     const std::vector<Vertex>& vertices,
	                                     const std::vector<uint32_t>& indices,
	                                     const VoxelizeSettings& settings)
	{
		return Voxelize(vertices, indices, settings);
	}

	VoxelMeshData VoxelizerAPI::Voxelize(const std::vector<Vertex>& vertices,
	                                     const std::vector<uint32_t>& indices,
	                                     const VoxelizeSettings& settings)
	{
		CB_PROFILE_FUNCTION();

		if (!s_Initialized) {
			CB_CORE_WARN("VoxelizerAPI::Voxelize called before Init()! Initializing now...");
			Init();
		}

		VoxelMeshData result;

		// Convert to voxelizer format
		std::vector<glm::vec3> positions;
		positions.reserve(vertices.size());
		for (const auto& v : vertices) { positions.push_back(v.Position); }

		// Create voxelizer
		voxelizer::Voxelizer vox(settings.GridSize);
		if (settings.VoxelSize > 0.0f) { vox.SetVoxelSize(settings.VoxelSize); }
		vox.SetPadding(settings.Padding);

		// Perform voxelization
		{
			CB_PROFILE_SCOPE("Voxelizer::Voxelize");
			result.Grid = vox.Voxelize(positions, indices, settings.Solid);
		}

		result.VoxelCount = result.Grid.CountFilled();

		CB_CORE_INFO("Voxelized mesh: {0} voxels, grid size: ({1}, {2}, {3})",
		             result.VoxelCount, result.Grid.size.x, result.Grid.size.y, result.Grid.size.z);

		// Create renderable mesh (only on main thread with GL context)
		if (settings.CreateGPUMesh) {
			CB_PROFILE_SCOPE("CreateMeshFromGrid");
			result.Mesh = CreateMeshFromGrid(result.Grid);
		}

		return result;
	}

	// Helper to load mesh from file
	static void LoadMeshFromFile(const String& filePath,
	                             std::vector<Vertex>& vertices,
	                             std::vector<uint32_t>& indices)
	{
		Assimp::Importer importer;
		importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 0.01f);
		const aiScene* scene = importer.ReadFile(filePath,
		                                         aiProcess_Triangulate |
		                                         aiProcess_GenSmoothNormals |
		                                         aiProcess_CalcTangentSpace |
		                                         aiProcess_JoinIdenticalVertices |
		                                         aiProcess_PreTransformVertices |
		                                         aiProcess_GlobalScale
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			CB_CORE_ERROR("VoxelizerAPI: Failed to load mesh: {0}", importer.GetErrorString());
			return;
		}

		std::function<void(aiNode*)> processNode = [&](aiNode* node)
		{
			for (unsigned int i = 0; i < node->mNumMeshes; i++) {
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

				for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
					Vertex vertex{};
					vertex.Position = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};

					if (mesh->HasNormals())
						vertex.Normal = {mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
					else
						vertex.Normal = {0.0f, 1.0f, 0.0f};

					if (mesh->mTextureCoords[0])
						vertex.TexCoords = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
					else
						vertex.TexCoords = {0.0f, 0.0f};

					if (mesh->HasTangentsAndBitangents()) {
						vertex.Tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z};
						vertex.Bitangent = {mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z};
					}
					else {
						vertex.Tangent = {1.0f, 0.0f, 0.0f};
						vertex.Bitangent = {0.0f, 1.0f, 0.0f};
					}

					vertex.Color = {1.0f, 1.0f, 1.0f};
					vertices.push_back(vertex);
				}

				for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
					aiFace& face = mesh->mFaces[j];
					for (unsigned int k = 0; k < face.mNumIndices; k++) {
						indices.push_back(vertexOffset + face.mIndices[k]);
					}
				}
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++) { processNode(node->mChildren[i]); }
		};

		processNode(scene->mRootNode);
	}

	VoxelMeshData VoxelizerAPI::VoxelizeFromFile(const String& filePath,const VoxelizeSettings& settings)
	{
		CB_PROFILE_FUNCTION();

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		LoadMeshFromFile(filePath, vertices, indices);

		if (vertices.empty())
			return {};

		return Voxelize(vertices, indices, settings);
	}

	// ============================================================================
	// Texture-Colored Voxelization (Optimized - processes triangles, not voxels)
	// ============================================================================

	void VoxelizerAPI::ComputeVoxelColors(const voxelizer::VoxelGrid& grid,
	                                      const std::vector<Vertex>& vertices,
	                                      const std::vector<uint32_t>& indices,
	                                      const TextureSampler& texture,
	                                      std::unordered_map<uint64_t, Vector3>& colorSumMap,
	                                      std::unordered_map<uint64_t, int>& colorCountMap)
	{
		glm::vec3 halfSize = grid.voxelSize * 0.5f;
		float boxhalfsize[3] = {halfSize.x, halfSize.y, halfSize.z};

		// Process each triangle - find which voxels it touches
		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			const Vertex& v0 = vertices[indices[i]];
			const Vertex& v1 = vertices[indices[i + 1]];
			const Vertex& v2 = vertices[indices[i + 2]];

			// Sample color once per triangle (average of vertex UVs)
			Vector2 centerUV = (v0.TexCoords + v1.TexCoords + v2.TexCoords) / 3.0f;
			Vector3 triColor = texture.SampleBilinear(centerUV);

			// Compute triangle AABB
			glm::vec3 triMin = min(min(v0.Position, v1.Position), v2.Position);
			glm::vec3 triMax = max(max(v0.Position, v1.Position), v2.Position);

			// Convert to voxel coordinates (with bounds clamping)
			glm::ivec3 voxMin = grid.WorldToVoxel(triMin);
			glm::ivec3 voxMax = grid.WorldToVoxel(triMax);

			voxMin = max(voxMin, glm::ivec3(0));
			voxMax = min(voxMax, grid.size - 1);

			// Prepare triangle vertices for collision test
			float triverts[3][3] = {
				{v0.Position.x, v0.Position.y, v0.Position.z},
				{v1.Position.x, v1.Position.y, v1.Position.z},
				{v2.Position.x, v2.Position.y, v2.Position.z}
			};

			// Only check voxels within triangle's AABB (huge optimization!)
			for (int x = voxMin.x; x <= voxMax.x; ++x) {
				for (int y = voxMin.y; y <= voxMax.y; ++y) {
					for (int z = voxMin.z; z <= voxMax.z; ++z) {
						uint64_t voxelIdx = grid.CoordToIndex(x, y, z);

						// Only process filled voxels
						if (!grid.IsFilled(voxelIdx))
							continue;

						glm::vec3 center = grid.VoxelCenterToWorld(glm::ivec3(x, y, z));
						float boxcenter[3] = {center.x, center.y, center.z};

						if (voxelizer::TriBoxOverlap(boxcenter, boxhalfsize, triverts)) {
							colorSumMap[voxelIdx] += triColor;
							colorCountMap[voxelIdx]++;
						}
					}
				}
			}
		}
	}

	VoxelMeshData VoxelizerAPI::VoxelizeWithColors(const std::vector<Vertex>& vertices,
	                                               const std::vector<uint32_t>& indices,
	                                               const String& texturePath,
	                                               const VoxelizeSettings& settings)
	{
		CB_PROFILE_FUNCTION();

		if (!s_Initialized) { Init(); }

		VoxelMeshData result;

		// Load texture for color sampling
		TextureSampler texture;
		if (!texture.Load(texturePath)) {
			CB_CORE_WARN("VoxelizerAPI: Failed to load texture, using default colors");
			return Voxelize(vertices, indices, settings);
		}

		// Convert to voxelizer format
		std::vector<glm::vec3> positions;
		positions.reserve(vertices.size());
		for (const auto& v : vertices) { positions.push_back(v.Position); }

		// Create voxelizer and perform voxelization
		voxelizer::Voxelizer vox(settings.GridSize);
		if (settings.VoxelSize > 0.0f) { vox.SetVoxelSize(settings.VoxelSize); }
		vox.SetPadding(settings.Padding);

		{
			CB_PROFILE_SCOPE("Voxelizer::Voxelize");
			result.Grid = vox.Voxelize(positions, indices, settings.Solid);
		}

		result.VoxelCount = result.Grid.CountFilled();

		// Compute colors efficiently - process triangles, not voxels
		std::unordered_map<uint64_t, Vector3> colorSumMap;
		std::unordered_map<uint64_t, int> colorCountMap;

		{
			CB_PROFILE_SCOPE("ComputeVoxelColors");
			ComputeVoxelColors(result.Grid, vertices, indices, texture, colorSumMap, colorCountMap);
		}

		// Build color array in same order as filled coords
		auto filledCoords = result.Grid.GetFilledCoords();
		std::vector<Vector3> voxelColors;
		voxelColors.reserve(filledCoords.size());

		for (const auto& coord : filledCoords) {
			uint64_t idx = result.Grid.CoordToIndex(coord);
			auto it = colorCountMap.find(idx);
			if (it != colorCountMap.end() && it->second > 0) {
				voxelColors.push_back(colorSumMap[idx] / static_cast<float>(it->second));
			}
			else {
				// Interior voxel with no triangle intersection - use average nearby color or default
				voxelColors.push_back(Vector3(0.5f, 0.5f, 0.5f));
			}
		}

		CB_CORE_INFO("Voxelized mesh with colors: {0} voxels, grid size: ({1}, {2}, {3})",
		             result.VoxelCount, result.Grid.size.x, result.Grid.size.y, result.Grid.size.z);

		// Create colored mesh (only on main thread with GL context)
		if (settings.CreateGPUMesh) {
			CB_PROFILE_SCOPE("CreateColoredMeshFromGrid");
			result.Mesh = CreateColoredMeshFromGrid(result.Grid, voxelColors);
		}

		return result;
	}

	VoxelMeshData VoxelizerAPI::VoxelizeFromFileWithColors(const String& meshPath,
	                                                       const String& texturePath,
	                                                       const VoxelizeSettings& settings)
	{
		CB_PROFILE_FUNCTION();

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		LoadMeshFromFile(meshPath, vertices, indices);

		if (vertices.empty())
			return {};

		return VoxelizeWithColors(vertices, indices, texturePath, settings);
	}

	Ref<Mesh> VoxelizerAPI::CreateColoredMeshFromGrid(const voxelizer::VoxelGrid& grid,
	                                                  const std::vector<Vector3>& voxelColors)
	{
		CB_PROFILE_FUNCTION();

		if (!s_Initialized) { Init(); }

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		auto filledCoords = grid.GetFilledCoords();

		if (filledCoords.size() != voxelColors.size()) {
			CB_CORE_ERROR("VoxelizerAPI: Color count mismatch! {0} voxels, {1} colors",
			              filledCoords.size(), voxelColors.size());
			return nullptr;
		}

		vertices.reserve(filledCoords.size() * 24);
		indices.reserve(filledCoords.size() * 36);

		glm::vec3 voxelSize = grid.voxelSize;

		for (size_t i = 0; i < filledCoords.size(); ++i) {
			const auto& coord = filledCoords[i];
			const Vector3& color = voxelColors[i];

			glm::vec3 center = grid.VoxelCenterToWorld(coord);

			// Emit only visible faces (neighbor culling)
			for (int face = 0; face < 6; ++face) {
				if (!ShouldEmitFace(grid, coord, face))
					continue;

				uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
				uint32_t faceVertStart = face * 4;

				for (int fv = 0; fv < 4; ++fv) {
					Vertex v = s_UnitCubeVertices[faceVertStart + fv];
					v.Position = center + Vector3(v.Position) * voxelSize;
					v.Color = color;
					vertices.push_back(v);
				}

				indices.push_back(baseVertex + 0);
				indices.push_back(baseVertex + 1);
				indices.push_back(baseVertex + 2);
				indices.push_back(baseVertex + 2);
				indices.push_back(baseVertex + 3);
				indices.push_back(baseVertex + 0);
			}
		}

		if (vertices.empty()) {
			CB_CORE_WARN("VoxelizerAPI::CreateColoredMeshFromGrid: No filled voxels");
			return nullptr;
		}

		return CreateRef<Mesh>(vertices, indices);
	}

	VoxelMeshData VoxelizerAPI::VoxelizeWithColorsAndOutput(const std::vector<Vertex>& vertices,
	                                                        const std::vector<uint32_t>& indices,
	                                                        const String& texturePath,
	                                                        const VoxelizeSettings& settings,
	                                                        std::vector<Vector3>& outVoxelColors)
	{
		CB_PROFILE_FUNCTION();

		if (!s_Initialized)
			Init();

		VoxelMeshData result;

		TextureSampler texture;
		if (!texture.Load(texturePath)) {
			CB_CORE_WARN("VoxelizerAPI: Failed to load texture, using default colors");
			return Voxelize(vertices, indices, settings);
		}

		std::vector<glm::vec3> positions;
		positions.reserve(vertices.size());
		for (const auto& v : vertices)
			positions.push_back(v.Position);

		voxelizer::Voxelizer vox(settings.GridSize);
		if (settings.VoxelSize > 0.0f)
			vox.SetVoxelSize(settings.VoxelSize);
		vox.SetPadding(settings.Padding);

		{
			CB_PROFILE_SCOPE("Voxelizer::Voxelize");
			result.Grid = vox.Voxelize(positions, indices, settings.Solid);
		}

		result.VoxelCount = result.Grid.CountFilled();

		std::unordered_map<uint64_t, Vector3> colorSumMap;
		std::unordered_map<uint64_t, int> colorCountMap;

		{
			CB_PROFILE_SCOPE("ComputeVoxelColors");
			ComputeVoxelColors(result.Grid, vertices, indices, texture, colorSumMap, colorCountMap);
		}

		auto filledCoords = result.Grid.GetFilledCoords();
		outVoxelColors.clear();
		outVoxelColors.reserve(filledCoords.size());

		for (const auto& coord : filledCoords) {
			uint64_t idx = result.Grid.CoordToIndex(coord);
			auto it = colorCountMap.find(idx);
			if (it != colorCountMap.end() && it->second > 0)
				outVoxelColors.push_back(colorSumMap[idx] / static_cast<float>(it->second));
			else
				outVoxelColors.push_back(Vector3(0.5f, 0.5f, 0.5f));
		}

		if (settings.CreateGPUMesh) {
			CB_PROFILE_SCOPE("CreateColoredMeshFromGrid");
			result.Mesh = CreateColoredMeshFromGrid(result.Grid, outVoxelColors);
		}

		return result;
	}

	// ============================================================================
	// UV Computation
	// ============================================================================

	void VoxelizerAPI::ComputeVoxelUVs(const voxelizer::VoxelGrid& grid,
	                                   const std::vector<Vertex>& vertices,
	                                   const std::vector<uint32_t>& indices,
	                                   std::unordered_map<uint64_t, Vector2>& uvSumMap,
	                                   std::unordered_map<uint64_t, int>& uvCountMap)
	{
		glm::vec3 halfSize = grid.voxelSize * 0.5f;
		float boxhalfsize[3] = {halfSize.x, halfSize.y, halfSize.z};

		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			const Vertex& v0 = vertices[indices[i]];
			const Vertex& v1 = vertices[indices[i + 1]];
			const Vertex& v2 = vertices[indices[i + 2]];

			Vector2 centerUV = (v0.TexCoords + v1.TexCoords + v2.TexCoords) / 3.0f;

			glm::vec3 triMin = min(min(v0.Position, v1.Position), v2.Position);
			glm::vec3 triMax = max(max(v0.Position, v1.Position), v2.Position);

			glm::ivec3 voxMin = grid.WorldToVoxel(triMin);
			glm::ivec3 voxMax = grid.WorldToVoxel(triMax);

			voxMin = max(voxMin, glm::ivec3(0));
			voxMax = min(voxMax, grid.size - 1);

			float triverts[3][3] = {
				{v0.Position.x, v0.Position.y, v0.Position.z},
				{v1.Position.x, v1.Position.y, v1.Position.z},
				{v2.Position.x, v2.Position.y, v2.Position.z}
			};

			for (int x = voxMin.x; x <= voxMax.x; ++x) {
				for (int y = voxMin.y; y <= voxMax.y; ++y) {
					for (int z = voxMin.z; z <= voxMax.z; ++z) {
						uint64_t voxelIdx = grid.CoordToIndex(x, y, z);

						if (!grid.IsFilled(voxelIdx))
							continue;

						glm::vec3 center = grid.VoxelCenterToWorld(glm::ivec3(x, y, z));
						float boxcenter[3] = {center.x, center.y, center.z};

						if (voxelizer::TriBoxOverlap(boxcenter, boxhalfsize, triverts)) {
							uvSumMap[voxelIdx] += centerUV;
							uvCountMap[voxelIdx]++;
						}
					}
				}
			}
		}
	}

	VoxelMeshData VoxelizerAPI::VoxelizeWithUVs(const std::vector<Vertex>& vertices,
	                                            const std::vector<uint32_t>& indices,
	                                            const VoxelizeSettings& settings,
	                                            std::vector<Vector2>& outVoxelUVs)
	{
		CB_PROFILE_FUNCTION();

		if (!s_Initialized)
			Init();

		VoxelMeshData result;

		std::vector<glm::vec3> positions;
		positions.reserve(vertices.size());
		for (const auto& v : vertices)
			positions.push_back(v.Position);

		voxelizer::Voxelizer vox(settings.GridSize);
		if (settings.VoxelSize > 0.0f)
			vox.SetVoxelSize(settings.VoxelSize);
		vox.SetPadding(settings.Padding);

		{
			CB_PROFILE_SCOPE("Voxelizer::Voxelize");
			result.Grid = vox.Voxelize(positions, indices, settings.Solid);
		}

		result.VoxelCount = result.Grid.CountFilled();

		// Compute per-voxel average UVs
		std::unordered_map<uint64_t, Vector2> uvSumMap;
		std::unordered_map<uint64_t, int> uvCountMap;

		{
			CB_PROFILE_SCOPE("ComputeVoxelUVs");
			ComputeVoxelUVs(result.Grid, vertices, indices, uvSumMap, uvCountMap);
		}

		auto filledCoords = result.Grid.GetFilledCoords();
		outVoxelUVs.clear();
		outVoxelUVs.reserve(filledCoords.size());

		for (const auto& coord : filledCoords) {
			uint64_t idx = result.Grid.CoordToIndex(coord);
			auto it = uvCountMap.find(idx);
			if (it != uvCountMap.end() && it->second > 0)
				outVoxelUVs.push_back(uvSumMap[idx] / static_cast<float>(it->second));
			else
				outVoxelUVs.push_back(Vector2(0.5f, 0.5f));
		}

		if (settings.CreateGPUMesh) {
			CB_PROFILE_SCOPE("CreateMeshFromGrid");
			result.Mesh = CreateMeshFromGrid(result.Grid);
		}

		return result;
	}

	VoxelPaletteData VoxelizerAPI::BuildPaletteFromUVs(const std::vector<Vector2>& voxelUVs,
	                                                   const String& texturePath)
	{
		CB_PROFILE_FUNCTION();

		TextureSampler texture;
		if (!texture.Load(texturePath)) {
			CB_CORE_WARN("VoxelizerAPI::BuildPaletteFromUVs: Failed to load texture {0}", texturePath);
			return {};
		}

		// Sample texture at each UV to get colors
		std::vector<Vector3> colors;
		colors.reserve(voxelUVs.size());
		for (const auto& uv : voxelUVs)
			colors.push_back(texture.SampleBilinear(uv));

		return BuildPaletteFromColors(colors);
	}

	// ============================================================================
	// Palette Methods
	// ============================================================================

	std::vector<Vector3> VoxelizerAPI::BuildPaletteMedianCut(const std::vector<Vector3>& colors,int maxColors)
	{
		CB_PROFILE_FUNCTION();

		if (colors.empty())
			return {};

		// Collect unique colors (or all if reasonable count)
		std::vector<Vector3> samples = colors;

		// A "box" in RGB space
		struct ColorBox
		{
			std::vector<Vector3> Colors;

			int LongestAxis() const
			{
				Vector3 minC(std::numeric_limits<float>::max());
				Vector3 maxC(std::numeric_limits<float>::lowest());
				for (const auto& c : Colors) {
					minC = glm::min(minC, c);
					maxC = glm::max(maxC, c);
				}
				Vector3 range = maxC - minC;
				if (range.r >= range.g && range.r >= range.b) return 0;
				if (range.g >= range.r && range.g >= range.b) return 1;
				return 2;
			}

			float AxisRange(int axis) const
			{
				float lo = std::numeric_limits<float>::max();
				float hi = std::numeric_limits<float>::lowest();
				for (const auto& c : Colors) {
					float v = c[axis];
					if (v < lo) lo = v;
					if (v > hi) hi = v;
				}
				return hi - lo;
			}

			Vector3 Average() const
			{
				Vector3 sum(0.0f);
				for (const auto& c : Colors)
					sum += c;
				return sum / static_cast<float>(Colors.size());
			}
		};

		// Start with one box containing all colors
		std::vector<ColorBox> boxes;
		boxes.push_back({std::move(samples)});

		// Split until we have enough boxes or can't split more
		while (static_cast<int>(boxes.size()) < maxColors) {
			// Find the box with the largest range on its longest axis
			int bestBox = -1;
			float bestRange = 0.0f;
			for (int i = 0; i < static_cast<int>(boxes.size()); i++) {
				if (boxes[i].Colors.size() < 2)
					continue;
				int axis = boxes[i].LongestAxis();
				float range = boxes[i].AxisRange(axis);
				if (range > bestRange) {
					bestRange = range;
					bestBox = i;
				}
			}

			if (bestBox < 0 || bestRange < 1e-6f)
				break; // Can't split further

			// Split the best box at the median of its longest axis
			int axis = boxes[bestBox].LongestAxis();
			auto& colors = boxes[bestBox].Colors;
			std::sort(colors.begin(), colors.end(), [axis](const Vector3& a,const Vector3& b)
			{
				return a[axis] < b[axis];
			});

			size_t mid = colors.size() / 2;
			ColorBox boxB;
			boxB.Colors.assign(colors.begin() + mid, colors.end());
			colors.resize(mid);

			boxes.push_back(std::move(boxB));
		}

		// Each box's average becomes a palette entry
		std::vector<Vector3> palette;
		palette.reserve(boxes.size());
		for (const auto& box : boxes)
			palette.push_back(box.Average());

		return palette;
	}

	VoxelPaletteData VoxelizerAPI::BuildPaletteFromColors(const std::vector<Vector3>& voxelColors)
	{
		CB_PROFILE_FUNCTION();

		VoxelPaletteData data;
		data.PaletteIndices.reserve(voxelColors.size());

		if (voxelColors.empty())
			return data;

		// Use median-cut to build a globally optimal palette
		std::vector<Vector3> paletteColors = BuildPaletteMedianCut(voxelColors,
		                                                           VoxelPalette::MaxEntries);

		// Add palette entries
		for (size_t i = 0; i < paletteColors.size(); i++) {
			VoxelPaletteEntry entry;
			entry.Color = paletteColors[i];
			if (i == 0)
				data.Palette.SetEntry(0, entry);
			else
				data.Palette.AddEntry(entry);
		}

		// Map each voxel to its closest palette entry
		for (const auto& color : voxelColors) {
			uint8_t bestIdx = data.Palette.FindClosestEntry(color);
			data.PaletteIndices.push_back(bestIdx);
		}

		return data;
	}

	Ref<Mesh> VoxelizerAPI::CreatePaletteMeshFromGrid(const voxelizer::VoxelGrid& grid,
	                                                  const std::vector<uint8_t>& paletteIndices,
	                                                  const VoxelTintMap* tintMap)
	{
		CB_PROFILE_SCOPE_CAT("VoxelizerAPI::CreatePaletteMeshFromGrid", "Voxel");

		if (!s_Initialized)
			Init();

		const uint64_t filledCount = grid.CountFilled();
		if (filledCount != paletteIndices.size()) {
			CB_CORE_ERROR("VoxelizerAPI: Palette index count mismatch! {0} voxels, {1} indices",
			              filledCount, paletteIndices.size());
			return nullptr;
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		// Most voxels expose ~3 faces on average (interior culling)
		vertices.reserve(filledCount * 12);  // ~3 faces * 4 verts
		indices.reserve(filledCount * 18);   // ~3 faces * 6 indices

		const glm::vec3 voxelSize = grid.voxelSize;
		const glm::vec3 origin = grid.origin;
		const int sizeX = grid.size.x, sizeY = grid.size.y, sizeZ = grid.size.z;
		const int strideYZ = sizeY * sizeZ;

		// Iterate the bit-packed data directly — no GetFilledCoords allocation
		size_t paletteIdx = 0;
		for (int x = 0; x < sizeX; ++x) {
			for (int y = 0; y < sizeY; ++y) {
				for (int z = 0; z < sizeZ; ++z) {
					const uint64_t linearIdx = static_cast<uint64_t>(x) * strideYZ
					                         + static_cast<uint64_t>(y) * sizeZ + z;
					if (!(grid.data[linearIdx >> 6] & (uint64_t(1) << (linearIdx & 63))))
						continue;

					const float palIdx = paletteIndices[paletteIdx++];
					const glm::ivec3 coord(x, y, z);
					const glm::vec3 center = origin + (glm::vec3(coord) + 0.5f) * voxelSize;

					// Tint lookup: once per voxel, not per vertex
					Vector3 tintColor(1.0f);
					if (tintMap) {
						auto it = tintMap->find(coord);
						if (it != tintMap->end())
							tintColor = glm::mix(Vector3(1.0f), Vector3(it->second.Color), it->second.Intensity);
					}

					// Check 6 faces with inlined neighbor test
					for (int face = 0; face < 6; ++face) {
						const glm::ivec3& dir = s_FaceDirections[face];
						int nx = x + dir.x, ny = y + dir.y, nz = z + dir.z;
						bool neighborFilled = false;
						if (nx >= 0 && nx < sizeX && ny >= 0 && ny < sizeY && nz >= 0 && nz < sizeZ) {
							uint64_t nIdx = static_cast<uint64_t>(nx) * strideYZ
							              + static_cast<uint64_t>(ny) * sizeZ + nz;
							neighborFilled = (grid.data[nIdx >> 6] & (uint64_t(1) << (nIdx & 63))) != 0;
						}
						if (neighborFilled) continue;

						uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
						uint32_t faceVertStart = face * 4;

						for (int fv = 0; fv < 4; ++fv) {
							Vertex v = s_UnitCubeVertices[faceVertStart + fv];
							v.Position = center + Vector3(v.Position) * voxelSize;
							v.Color = tintColor;
							v.PaletteIndex = palIdx;
							vertices.push_back(v);
						}

						indices.push_back(baseVertex + 0);
						indices.push_back(baseVertex + 1);
						indices.push_back(baseVertex + 2);
						indices.push_back(baseVertex + 2);
						indices.push_back(baseVertex + 3);
						indices.push_back(baseVertex + 0);
					}
				}
			}
		}

		if (vertices.empty()) {
			CB_CORE_WARN("VoxelizerAPI::CreatePaletteMeshFromGrid: No filled voxels");
			return nullptr;
		}

		return CreateRef<Mesh>(vertices, indices, true);
	}

	// =========================================================================
	// RecolorPaletteMesh — fast vertex-color-only update (no geometry rebuild)
	// =========================================================================
	bool VoxelizerAPI::RecolorPaletteMesh(Ref<Mesh>& mesh,
	                                      const voxelizer::VoxelGrid& grid,
	                                      const std::vector<uint8_t>& paletteIndices,
	                                      const VoxelTintMap& tintMap)
	{
		CB_PROFILE_SCOPE_CAT("VoxelizerAPI::RecolorPaletteMesh", "Voxel");

		if (!mesh || !mesh->HasCPUData()) return false;

		auto& vertices = mesh->GetMutableVertices();
		if (vertices.empty()) return false;

		const int sizeX = grid.size.x, sizeY = grid.size.y, sizeZ = grid.size.z;
		const int strideYZ = sizeY * sizeZ;

		// Walk the grid in the same order as CreatePaletteMeshFromGrid,
		// matching each visible face to its 4 consecutive vertices.
		size_t vertIdx = 0;
		for (int x = 0; x < sizeX; ++x) {
			for (int y = 0; y < sizeY; ++y) {
				for (int z = 0; z < sizeZ; ++z) {
					const uint64_t linearIdx = static_cast<uint64_t>(x) * strideYZ
					                         + static_cast<uint64_t>(y) * sizeZ + z;
					if (!(grid.data[linearIdx >> 6] & (uint64_t(1) << (linearIdx & 63))))
						continue;

					const glm::ivec3 coord(x, y, z);

					// Compute tint color for this voxel
					Vector3 tintColor(1.0f);
					auto it = tintMap.find(coord);
					if (it != tintMap.end())
						tintColor = glm::mix(Vector3(1.0f), Vector3(it->second.Color), it->second.Intensity);

					// Check same 6 faces in same order as CreatePaletteMeshFromGrid
					for (int face = 0; face < 6; ++face) {
						const glm::ivec3& dir = s_FaceDirections[face];
						int nx = x + dir.x, ny = y + dir.y, nz = z + dir.z;
						bool neighborFilled = false;
						if (nx >= 0 && nx < sizeX && ny >= 0 && ny < sizeY && nz >= 0 && nz < sizeZ) {
							uint64_t nIdx = static_cast<uint64_t>(nx) * strideYZ
							              + static_cast<uint64_t>(ny) * sizeZ + nz;
							neighborFilled = (grid.data[nIdx >> 6] & (uint64_t(1) << (nIdx & 63))) != 0;
						}
						if (neighborFilled) continue;

						// This face has 4 consecutive vertices — update their colors
						if (vertIdx + 3 >= vertices.size()) return false; // mismatch guard
						vertices[vertIdx].Color = tintColor;
						vertices[vertIdx + 1].Color = tintColor;
						vertices[vertIdx + 2].Color = tintColor;
						vertices[vertIdx + 3].Color = tintColor;
						vertIdx += 4;
					}
				}
			}
		}

		mesh->UploadVertexData();
		return true;
	}

	// ============================================================================
	// Standard Methods
	// ============================================================================

	Ref<Mesh> VoxelizerAPI::CreateMeshFromGrid(const voxelizer::VoxelGrid& grid,const VoxelTintMap* tintMap)
	{
		return CreateMeshFromGrid(grid, Vector3(1.0f, 1.0f, 1.0f), tintMap);
	}

	Ref<Mesh> VoxelizerAPI::CreateMeshFromGrid(const voxelizer::VoxelGrid& grid,const Vector3& color,
	                                           const VoxelTintMap* tintMap)
	{
		CB_PROFILE_FUNCTION();

		if (!s_Initialized)
			Init();

		auto filledCoords = grid.GetFilledCoords();
		if (filledCoords.empty()) {
			CB_CORE_WARN("VoxelizerAPI::CreateMeshFromGrid: No filled voxels");
			return nullptr;
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(filledCoords.size() * 24);
		indices.reserve(filledCoords.size() * 36);

		glm::vec3 voxelSize = grid.voxelSize;

		for (const auto& coord : filledCoords) {
			glm::vec3 center = grid.VoxelCenterToWorld(coord);

			Vector3 tintColor(1.0f);
			if (tintMap) {
				auto tintIt = tintMap->find(coord);
				if (tintIt != tintMap->end())
					tintColor = glm::mix(Vector3(1.0f), Vector3(tintIt->second.Color), tintIt->second.Intensity);
			}

			// Emit only visible faces (neighbor culling)
			for (int face = 0; face < 6; ++face) {
				if (!ShouldEmitFace(grid, coord, face))
					continue;

				uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
				uint32_t faceVertStart = face * 4;

				for (int fv = 0; fv < 4; ++fv) {
					Vertex v = s_UnitCubeVertices[faceVertStart + fv];
					v.Position = center + Vector3(v.Position) * voxelSize;
					v.Color = color * tintColor;
					vertices.push_back(v);
				}

				indices.push_back(baseVertex + 0);
				indices.push_back(baseVertex + 1);
				indices.push_back(baseVertex + 2);
				indices.push_back(baseVertex + 2);
				indices.push_back(baseVertex + 3);
				indices.push_back(baseVertex + 0);
			}
		}

		return CreateRef<Mesh>(vertices, indices);
	}

	std::vector<Vector3> VoxelizerAPI::GetVoxelPositions(const voxelizer::VoxelGrid& grid)
	{
		auto glmPositions = grid.GetFilledPositions();
		std::vector<Vector3> positions;
		positions.reserve(glmPositions.size());
		for (const auto& p : glmPositions) { positions.push_back(p); }
		return positions;
	}

	Vector3 VoxelizerAPI::GetVoxelSize(const voxelizer::VoxelGrid& grid) { return grid.voxelSize; }

	bool VoxelizerAPI::IsPointInside(const voxelizer::VoxelGrid& grid,const Vector3& worldPos)
	{
		glm::ivec3 voxelCoord = grid.WorldToVoxel(worldPos);
		return grid.IsFilled(voxelCoord);
	}

	Ref<Shader> VoxelizerAPI::GetVoxelShader()
	{
		if (!s_VoxelShader) { s_VoxelShader = Shader::Create("assets/shaders/PBR.glsl"); }
		return s_VoxelShader;
	}

	Ref<Material> VoxelizerAPI::CreateVoxelMaterial(const Vector3& color)
	{
		auto material = CreateRef<Material>();
		material->SetAlbedo(color);
		material->SetRoughness(0.7f);
		material->SetMetallic(0.0f);
		return material;
	}
}