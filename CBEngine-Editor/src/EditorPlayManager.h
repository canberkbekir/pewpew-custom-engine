#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Core/Timestep.h"

namespace CB
{
	enum class EditorPlayState { Edit, Play, Ejected };

	struct EditorPlaySettings
	{
		bool StartWithCursorLocked = true;
	};

	class PerspectiveCameraController;

	class EditorPlayManager
	{
	public:
		EditorPlayManager() { s_Instance = this; }
		~EditorPlayManager();

		void Init();
		void Shutdown();

		// State transitions
		void Play(PerspectiveCameraController& editorCam);
		void Stop(PerspectiveCameraController& editorCam);
		void Eject(PerspectiveCameraController& editorCam);
		void ResumePlay();

		// Queries (cheap, inline)
		EditorPlayState GetState() const { return m_State; }
		bool IsSimulating() const { return m_State != EditorPlayState::Edit; }
		bool ShouldUseEditorCamera() const { return m_State != EditorPlayState::Play; }
		bool ShouldShowGizmos() const { return m_State != EditorPlayState::Play; }
		bool ShouldCaptureInput() const { return m_State == EditorPlayState::Play; }

		EditorPlaySettings& GetSettings() { return m_Settings; }

		static EditorPlayManager& Get() { return *s_Instance; }

	private:
		static EditorPlayManager* s_Instance;

		EditorPlayState m_State = EditorPlayState::Edit;
		EditorPlaySettings m_Settings;

		// Cached editor camera transform for restore on Stop
		Vector3 m_CachedCamPos = {0, 0, 0};
		float m_CachedCamPitch = 0.0f;
		float m_CachedCamYaw = 0.0f;
	};
}
