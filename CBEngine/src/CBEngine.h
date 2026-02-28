#pragma once

// For use by CB applications

// ---Core----------------------------
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/Application.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Core/LogBuffer.h"
#include "CBEngine/Core/Layer.h"
#include "CBEngine/Core/TimeStep.h"
// -----------------------------------

// ---Input---------------------------
#include "CBEngine/Input/Input.h"
#include "CBEngine/Input/KeyCodes.h"
#include "CBEngine/Input/MouseButtonCodes.h"
// -----------------------------------

// ---Events--------------------------
#include "CBEngine/Events/Event.h"
#include "CBEngine/Events/ApplicationEvent.h"
#include "CBEngine/Events/KeyEvent.h"
#include "CBEngine/Events/MouseEvent.h"
#include "CBEngine/Events/AssetEvent.h"
#include "CBEngine/Events/SceneEvent.h"
#include "CBEngine/Events/EditorEvent.h"
// -----------------------------------

// ---Debug---------------------------
#include "CBEngine/Debug/Instrumentor.h"
// -----------------------------------

// ---Renderer------------------------
#include "CBEngine/Renderer/Core/Renderer.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"

#include "CBEngine/Renderer/Resources/Buffer.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Renderer/Resources/VertexArray.h"

#include "CBEngine/Renderer/Camera/Camera.h"
#include "CBEngine/Renderer/Camera/OrthographicCamera.h"
#include "CBEngine/Renderer/Camera/PerspectiveCamera.h"
#include "CBEngine/Renderer/Camera/PerspectiveCameraController.h"

#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Material.h"
// -----------------------------------

// ---Utils---------------------------
#include "CBEngine/Voxel/VoxelizerAPI.h"
// -----------------------------------

// ---Scene---------------------------
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/SceneManager.h"
// -----------------------------------

// ---ImGui---------------------------
#include "CBEngine/ImGui/ImGuiLayer.h"
#include "CBEngine/ImGui/ImGuiUtils.h"
// -----------------------------------

// ---Entry Point---------------------
#include "CBEngine/Core/EntryPoint.h"
// -----------------------------------