#pragma once

//For use by PewPew applications
#include "PewPew/Core.h"
#include "PewPew/Application.h"
#include "PewPew/Log.h"
#include "PewPew/Layer.h"

#include "PewPew/Core/Timestep.h"

#include "PewPew/Input.h"
#include "PewPew/KeyCodes.h"
#include "PewPew/MouseButtonCodes.h"

// ---Debug---------------------------
#include "PewPew/Debug/Instrumentor.h"
#include "PewPew/Debug/ProfilerPanel.h"
// -----------------------------------

// ---Renderer------------------------
#include "PewPew/Renderer/Core/Renderer.h"
#include "PewPew/Renderer/Core/Renderer3D.h"
#include "PewPew/Renderer/Core/RenderCommand.h"

#include "PewPew/Renderer/Resources/Buffer.h"
#include "PewPew/Renderer/Resources/Shader.h"
#include "PewPew/Renderer/Resources/Texture.h"
#include "PewPew/Renderer/Resources/VertexArray.h"

#include "PewPew/Renderer/Camera/Camera.h"
#include "PewPew/Renderer/Camera/OrthographicCamera.h"
#include "PewPew/Renderer/Camera/PerspectiveCamera.h"
#include "PewPew/Renderer/Camera/PerspectiveCameraController.h"

#include "PewPew/Renderer/Resources/Mesh.h"
#include "PewPew/Renderer/Resources/Material.h"
// -----------------------------------

#include "PewPew/ImGui/ImGuiLayer.h"
//Entry Point-----------------
#include "PewPew/EntryPoint.h"
//---------------------------
