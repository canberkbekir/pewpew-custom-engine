#pragma once
#include "Camera.h"
#include "RendererAPI.h"
#include "Shader.h"
#include "VertexArray.h"

namespace PewPew {

	class Renderer
	{
	public:
		static void Init();
		static void BeginScene(const Camera& camera);
		static void EndScene();

		static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const Mat4& transform = Mat4(1.0f));

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			Mat4 ViewProjectionMatrix;
		};

		static SceneData* s_SceneData;
	};


}