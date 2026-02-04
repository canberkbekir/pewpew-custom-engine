#pragma once

// For use by PewPew applications

// ---Core----------------------------
#include "PewPew/Core/Core.h"
#include "PewPew/Core/Application.h"
#include "PewPew/Core/Log.h"
#include "PewPew/Core/LogBuffer.h"
#include "PewPew/Core/Layer.h"
#include "PewPew/Core/TimeStep.h"
// -----------------------------------

// ---Input---------------------------
#include "PewPew/Input/Input.h"
#include "PewPew/Input/KeyCodes.h"
#include "PewPew/Input/MouseButtonCodes.h"
// -----------------------------------

// ---Debug---------------------------
#include "PewPew/Debug/Instrumentor.h"
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

// ---Utils---------------------------
#include "PewPew/Utils/VoxelizerAPI.h"
// -----------------------------------

// ---Scene---------------------------
#include "PewPew/Scene/Scene.h"
#include "PewPew/Scene/Entity.h"
#include "PewPew/Scene/SceneManager.h"
// -----------------------------------

// ---ImGui---------------------------
#include "PewPew/ImGui/ImGuiLayer.h"
#include "PewPew/ImGui/ImGuiUtils.h"
// -----------------------------------

// ---Entry Point---------------------
#include "PewPew/Core/EntryPoint.h"
// -----------------------------------
