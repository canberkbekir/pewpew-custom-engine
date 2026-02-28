#include "cbpch.h"
#include "Application.h"

#include "Log.h"
#include "CBEngine/Debug/Instrumentor.h"

#include "CBEngine/Input/Input.h"
#include "TimeStep.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "CBEngine/Renderer/Core/Renderer.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "CBEngine/Voxel/Destruction/SubstanceRegistry.h"

// Forward declaration — forces the linker to include ComponentRegistrations.obj
// from CBEngine.lib (MSVC dead-strips objects with no referenced symbols).
namespace CB
{
	void LinkComponentRegistrations();
}

namespace CB
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		CB_PROFILE_FUNCTION();

		CB_CORE_ASSERT(!s_Instance, "Application already exists!")
		s_Instance = this;

		LinkComponentRegistrations();

		m_Window = Window::Create();
		m_Window->SetEventCallback(CB_BIND_EVENT_FN(OnEvent));

		Renderer::Init();
		Renderer3D::Init();
		PhysicsWorld::InitJoltGlobals();
		ScriptEngine::Init();

		AssetManager::Init("assets");
		SubstanceRegistry::Load("assets/config/voxel_substances.yaml");

#ifdef CB_ENABLE_IMGUI
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
#endif
	}

	Application::~Application()
	{
		CB_PROFILE_FUNCTION();
		AssetManager::Shutdown();
		ScriptEngine::Shutdown();
		PhysicsWorld::ShutdownJoltGlobals();
		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); }

	void Application::PushOverlay(Layer* layer) { m_LayerStack.PushOverlay(layer); }

	void Application::OnEvent(Event& e)
	{
		CB_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(CB_BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(CB_BIND_EVENT_FN(OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();) {
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		while (m_Running) {
			CB_PROFILE_BEGIN_FRAME();
			CB_PROFILE_SCOPE("RunLoop");

			float time = static_cast<float>(glfwGetTime());
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			// Snapshot previous input state for JustPressed/Released/MouseDelta
			Input::Update();

			// Process asset hot reloads
			AssetManager::ProcessReloadQueue();

			if (!m_Minimized) {
				{
					CB_PROFILE_SCOPE_CAT("LayerStack OnUpdate", "Scene");
					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}
			}

#ifdef CB_ENABLE_IMGUI
			{
				CB_PROFILE_SCOPE_CAT("LayerStack OnImGuiRender", "Editor");
				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->End();
			}
#endif

			m_Window->OnUpdate();
			CB_PROFILE_END_FRAME();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetHeight() == 0 || e.GetWidth() == 0) {
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
		return false;
	}
}