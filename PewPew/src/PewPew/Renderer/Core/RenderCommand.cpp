#include "pewpch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace PewPew
{
	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create(); 
}
