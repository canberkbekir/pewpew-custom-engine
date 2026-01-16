#include "pewpch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace PewPew {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;

}