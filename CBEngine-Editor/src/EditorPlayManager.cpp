#include "EditorPlayManager.h"

#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Input/Input.h"
#include "CBEngine/Components/CameraComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Renderer/Camera/PerspectiveCameraController.h"
#include "CBEngine/Core/Log.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace CB
{
	EditorPlayManager* EditorPlayManager::s_Instance = nullptr;

	EditorPlayManager::~EditorPlayManager()
	{
		if (s_Instance == this)
			s_Instance = nullptr;
	}

	void EditorPlayManager::Init()
	{
		// Auto-stop play mode if scene changes (e.g. Lua Scene.LoadScene)
		SceneManager::AddSceneChangedCallback([this](const Ref<Scene>&)
		{
			if (IsSimulating())
			{
				CB_CORE_WARN("Scene changed during play mode — auto-stopping.");
				// Scene already changed, just reset state (physics was shut down by the old scene)
				m_State = EditorPlayState::Edit;
				Input::SetCursorLocked(false);
				Input::SetGameInputBlocked(false);
			}
		});
	}

	void EditorPlayManager::Shutdown()
	{
		if (s_Instance == this)
			s_Instance = nullptr;
	}

	void EditorPlayManager::Play(PerspectiveCameraController& editorCam)
	{
		if (m_State != EditorPlayState::Edit)
			return;

		Ref<Scene> scene = SceneManager::GetActiveScene();
		if (!scene)
			return;

		// Cache editor camera transform for restore on Stop
		m_CachedCamPos = editorCam.GetCamera().GetPosition();
		m_CachedCamPitch = editorCam.GetCamera().GetPitch();
		m_CachedCamYaw = editorCam.GetCamera().GetYaw();

		// Start simulation
		scene->InitPhysics();

		// Lock cursor for gameplay
		if (m_Settings.StartWithCursorLocked)
			Input::SetCursorLocked(true);

		m_State = EditorPlayState::Play;
		CB_CORE_INFO("Play mode started.");
	}

	void EditorPlayManager::Stop(PerspectiveCameraController& editorCam)
	{
		if (m_State == EditorPlayState::Edit)
			return;

		Ref<Scene> scene = SceneManager::GetActiveScene();
		if (scene && scene->IsPhysicsInitialized())
			scene->ShutdownPhysics(); // also unlocks cursor

		// Restore editor camera to cached position
		editorCam.GetCamera().SetPosition(m_CachedCamPos);
		editorCam.GetCamera().SetRotation(m_CachedCamPitch, m_CachedCamYaw);

		Input::SetCursorLocked(false);
		Input::SetGameInputBlocked(false);

		m_State = EditorPlayState::Edit;
		CB_CORE_INFO("Play mode stopped.");
	}

	void EditorPlayManager::Eject(PerspectiveCameraController& editorCam)
	{
		if (m_State != EditorPlayState::Play)
			return;

		Ref<Scene> scene = SceneManager::GetActiveScene();
		if (!scene)
			return;

		// Find primary camera entity and copy its transform to editor camera
		auto view = scene->GetRegistry().view<TransformComponent, CameraComponent>();
		for (auto entity : view)
		{
			auto& cam = view.get<CameraComponent>(entity);
			if (!cam.Primary)
				continue;

			auto& tc = view.get<TransformComponent>(entity);
			Vector3 worldPos = tc.GetWorldPosition();
			editorCam.GetCamera().SetPosition(worldPos);

			// Extract world-space forward from WorldMatrix (same as GameViewportPanel)
			Vector3 forward = glm::normalize(Vector3(tc.WorldMatrix[2]));
			float pitch = glm::degrees(asin(glm::clamp(forward.y, -1.0f, 1.0f)));
			float yaw = glm::degrees(atan2(forward.z, forward.x));
			editorCam.GetCamera().SetRotation(pitch, yaw);
			break;
		}

		// Unlock cursor for editor navigation and block game input
		Input::SetCursorLocked(false);
		Input::SetGameInputBlocked(true);

		m_State = EditorPlayState::Ejected;
		CB_CORE_INFO("Ejected from play mode — editor camera active, simulation continues.");
	}

	void EditorPlayManager::ResumePlay()
	{
		if (m_State != EditorPlayState::Ejected)
			return;

		// Restore game input and lock cursor for gameplay
		Input::SetGameInputBlocked(false);
		if (m_Settings.StartWithCursorLocked)
			Input::SetCursorLocked(true);

		m_State = EditorPlayState::Play;
		CB_CORE_INFO("Resumed play mode — player camera active.");
	}
}
