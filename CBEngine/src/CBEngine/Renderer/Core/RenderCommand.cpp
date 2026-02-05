#include "cbpch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace CB
{
	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create(); 
}
