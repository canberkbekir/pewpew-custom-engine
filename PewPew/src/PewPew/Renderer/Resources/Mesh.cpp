#include "pewpch.h"
#include "Mesh.h"

#include "Buffer.h"
#include "PewPew/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace PewPew
{
	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		m_IndexCount = static_cast<uint32_t>(indices.size());

		m_VertexArray.reset(VertexArray::Create());

		// Create vertex buffer
		Ref<VertexBuffer> vertexBuffer;
		vertexBuffer.reset(VertexBuffer::Create(
			(float*)vertices.data(),
			static_cast<uint32_t>(vertices.size() * sizeof(Vertex))
		));

		vertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Bitangent" }
		});

		m_VertexArray->AddVertexBuffer(vertexBuffer);

		// Create index buffer
		Ref<IndexBuffer> indexBuffer;
		indexBuffer.reset(IndexBuffer::Create(
			(uint32_t*)indices.data(),
			m_IndexCount
		));

		m_VertexArray->SetIndexBuffer(indexBuffer);
	}

	static void ProcessMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		// Track vertex offset for index adjustment when combining multiple meshes
		uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

		// Process vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex{};

			// Position
			vertex.Position = {
				mesh->mVertices[i].x,
				mesh->mVertices[i].y,
				mesh->mVertices[i].z
			};

			// Normal
			if (mesh->HasNormals())
			{
				vertex.Normal = {
					mesh->mNormals[i].x,
					mesh->mNormals[i].y,
					mesh->mNormals[i].z
				};
			}
			else
			{
				vertex.Normal = { 0.0f, 1.0f, 0.0f };
			}

			// Texture coordinates (first set only)
			if (mesh->mTextureCoords[0])
			{
				vertex.TexCoords = {
					mesh->mTextureCoords[0][i].x,
					mesh->mTextureCoords[0][i].y
				};
			}
			else
			{
				vertex.TexCoords = { 0.0f, 0.0f };
			}

			// Tangent and Bitangent (for normal mapping)
			if (mesh->HasTangentsAndBitangents())
			{
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
			else
			{
				vertex.Tangent = { 1.0f, 0.0f, 0.0f };
				vertex.Bitangent = { 0.0f, 1.0f, 0.0f };
			}

			vertices.push_back(vertex);
		}

		// Process indices - add vertex offset to account for previously added vertices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(vertexOffset + face.mIndices[j]);
			}
		}
	}

	static void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		// Process all meshes in this node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(mesh, vertices, indices);
		}

		// Process child nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, vertices, indices);
		}
	}

	Ref<Mesh> Mesh::Load(const String& filePath)
	{
		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(filePath,
			aiProcess_Triangulate |           // Triangulate all faces
			aiProcess_GenSmoothNormals |      // Generate normals if missing
			aiProcess_CalcTangentSpace |      // Calculate tangents (for normal mapping later)
			aiProcess_JoinIdenticalVertices | // Optimize vertex count
			aiProcess_PreTransformVertices    // Bake node transforms into vertices
			// Note: NOT using aiProcess_FlipUVs - stb_image already flips textures
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			PEW_CORE_ERROR("Assimp Error: {0}", importer.GetErrorString());
			return nullptr;
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		// Process all nodes recursively
		ProcessNode(scene->mRootNode, scene, vertices, indices);

		PEW_CORE_INFO("Loaded mesh: {0} ({1} vertices, {2} triangles)",
			filePath, vertices.size(), indices.size() / 3);

		return std::make_shared<Mesh>(vertices, indices);
	}

	void Mesh::Bind() const
	{
		m_VertexArray->Bind();
	}
}
