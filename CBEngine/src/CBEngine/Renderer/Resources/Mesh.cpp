#include "cbpch.h"
#include "Mesh.h"

#include "Buffer.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Debug/Instrumentor.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

namespace CB
{
	static void ComputeBoundsFromVertices(const std::vector<Vertex>& vertices,
	                                      Vector3& outMin,Vector3& outMax)
	{
		outMin = Vector3(std::numeric_limits<float>::max());
		outMax = Vector3(std::numeric_limits<float>::lowest());
		for (const auto& v : vertices) {
			outMin = glm::min(outMin, v.Position);
			outMax = glm::max(outMax, v.Position);
		}
	}

	Mesh::Mesh(const std::vector<Vertex>& vertices,const std::vector<uint32_t>& indices,bool retainCPUData)
		: m_IsFromFile(false), m_RetainCPUData(retainCPUData)
	{
		CB_PROFILE_FUNCTION();
		m_Type = AssetType::Mesh;

		m_IndexCount = static_cast<uint32_t>(indices.size());
		m_VertexCount = static_cast<uint32_t>(vertices.size());

		// Compute bounding box before vertices might be discarded
		if (!vertices.empty()) {
			ComputeBoundsFromVertices(vertices, m_BoundsMin, m_BoundsMax);
			m_HasBounds = true;
		}

		if (m_RetainCPUData) {
			m_Vertices = vertices;
			m_Indices = indices;
		}

		m_VertexArray = VertexArray::Create();

		// Create vertex buffer
		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create((float*)vertices.data(),
		                                                      static_cast<uint32_t>(vertices.size() * sizeof(Vertex)));

		vertexBuffer->SetLayout({
			{ShaderDataType::Float3, "a_Position"},
			{ShaderDataType::Float3, "a_Normal"},
			{ShaderDataType::Float2, "a_TexCoord"},
			{ShaderDataType::Float3, "a_Tangent"},
			{ShaderDataType::Float3, "a_Bitangent"},
			{ShaderDataType::Float3, "a_Color"},
			{ShaderDataType::Float, "a_PaletteIndex"}
		});

		m_VertexArray->AddVertexBuffer(vertexBuffer);

		// Create index buffer
		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(const_cast<uint32_t*>(indices.data()), m_IndexCount);

		m_VertexArray->SetIndexBuffer(indexBuffer);
	}

	static void ProcessMesh(aiMesh* mesh,std::vector<Vertex>& vertices,std::vector<uint32_t>& indices)
	{
		// Track vertex offset for index adjustment when combining multiple meshes
		uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

		// Process vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex{};

			// Position
			vertex.Position = {
				mesh->mVertices[i].x,
				mesh->mVertices[i].y,
				mesh->mVertices[i].z
			};

			// Normal
			if (mesh->HasNormals()) {
				vertex.Normal = {
					mesh->mNormals[i].x,
					mesh->mNormals[i].y,
					mesh->mNormals[i].z
				};
			}
			else { vertex.Normal = {0.0f, 1.0f, 0.0f}; }

			// Texture coordinates (first set only)
			if (mesh->mTextureCoords[0]) {
				vertex.TexCoords = {
					mesh->mTextureCoords[0][i].x,
					mesh->mTextureCoords[0][i].y
				};
			}
			else { vertex.TexCoords = {0.0f, 0.0f}; }

			// Tangent and Bitangent (for normal mapping)
			if (mesh->HasTangentsAndBitangents()) {
				vertex.Tangent = {
					mesh->mTangents[i].x,
					mesh->mTangents[i].y,
					mesh->mTangents[i].z
				};
				vertex.Bitangent = {
					mesh->mBitangents[i].x,
					mesh->mBitangents[i].y,
					mesh->mBitangents[i].z
				};
			}
			else {
				vertex.Tangent = {1.0f, 0.0f, 0.0f};
				vertex.Bitangent = {0.0f, 1.0f, 0.0f};
			}

			vertices.push_back(vertex);
		}

		// Process indices - add vertex offset to account for previously added vertices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) { indices.push_back(vertexOffset + face.mIndices[j]); }
		}
	}

	static void ProcessNode(aiNode* node,const aiScene* scene,std::vector<Vertex>& vertices,
	                        std::vector<uint32_t>& indices)
	{
		// Process all meshes in this node
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(mesh, vertices, indices);
		}

		// Process child nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ProcessNode(node->mChildren[i], scene, vertices, indices);
		}
	}

	Ref<Mesh> Mesh::Load(const String& filePath) { return Load(filePath, false); }

	Ref<Mesh> Mesh::Load(const String& filePath,bool retainCPUData)
	{
		CB_PROFILE_FUNCTION();

		Assimp::Importer importer;

		// Scale from centimeters to meters (most 3D tools export in cm)
		importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 0.01f);

		const aiScene* scene = nullptr;
		{
			CB_PROFILE_SCOPE("Mesh::Assimp::ReadFile");
			scene = importer.ReadFile(filePath,
			                          aiProcess_Triangulate | // Triangulate all faces
			                          aiProcess_GenSmoothNormals | // Generate normals if missing
			                          aiProcess_CalcTangentSpace |
			                          // Calculate tangents (for normal mapping later)
			                          aiProcess_JoinIdenticalVertices | // Optimize vertex count
			                          aiProcess_PreTransformVertices | // Bake node transforms into vertices
			                          aiProcess_GlobalScale // Apply global scale factor (cm -> m)
			                          // Note: NOT using aiProcess_FlipUVs - stb_image already flips textures
			);
		}

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			CB_CORE_ERROR("Assimp Error: {0}", importer.GetErrorString());
			return nullptr;
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		// Process all nodes recursively
		{
			CB_PROFILE_SCOPE("Mesh::ProcessNode");
			ProcessNode(scene->mRootNode, scene, vertices, indices);
		}

		CB_CORE_INFO("Loaded mesh: {0} ({1} vertices, {2} triangles)",
		             filePath, vertices.size(), indices.size() / 3);

		auto mesh = std::make_shared<Mesh>(vertices, indices, retainCPUData);
		mesh->m_FilePath = filePath;
		mesh->m_IsFromFile = true;
		return mesh;
	}

	bool Mesh::LoadFromFile(const String& filePath)
	{
		CB_PROFILE_FUNCTION();

		Assimp::Importer importer;

		// Scale from centimeters to meters
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
			CB_CORE_ERROR("Assimp Error: {0}", importer.GetErrorString());
			return false;
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		ProcessNode(scene->mRootNode, scene, vertices, indices);

		m_IndexCount = static_cast<uint32_t>(indices.size());
		m_VertexCount = static_cast<uint32_t>(vertices.size());

		// Compute bounding box before vertices might be discarded
		if (!vertices.empty()) {
			ComputeBoundsFromVertices(vertices, m_BoundsMin, m_BoundsMax);
			m_HasBounds = true;
		}

		if (m_RetainCPUData) {
			m_Vertices = vertices;
			m_Indices = indices;
		}

		m_VertexArray = VertexArray::Create();

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create((float*)vertices.data(),
		                                                      static_cast<uint32_t>(vertices.size() * sizeof(Vertex)));
		vertexBuffer->SetLayout({
			{ShaderDataType::Float3, "a_Position"},
			{ShaderDataType::Float3, "a_Normal"},
			{ShaderDataType::Float2, "a_TexCoord"},
			{ShaderDataType::Float3, "a_Tangent"},
			{ShaderDataType::Float3, "a_Bitangent"},
			{ShaderDataType::Float3, "a_Color"},
			{ShaderDataType::Float, "a_PaletteIndex"}
		});
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), m_IndexCount);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		return true;
	}

	bool Mesh::Reload()
	{
		if (!m_IsFromFile || m_FilePath.empty()) {
			CB_CORE_WARN("Cannot reload mesh: not loaded from file");
			return false;
		}

		CB_PROFILE_FUNCTION();

		if (LoadFromFile(m_FilePath)) {
			CB_CORE_INFO("Reloaded mesh: {0}", m_FilePath);
			return true;
		}

		CB_CORE_ERROR("Failed to reload mesh: {0}", m_FilePath);
		return false;
	}

	void Mesh::Bind() const { m_VertexArray->Bind(); }
}