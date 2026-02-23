#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"
#include "AudioClipAsset.h"

#include <string>
#include <unordered_map>
#include <array>

// Forward-declare OpenAL types
typedef unsigned int ALuint;
typedef int ALint;

namespace CB
{
	// -------------------------------------------------------------------------
	// AudioEngine — thin OpenAL-Soft wrapper.
	//
	// Source pool: 32 pre-allocated AL sources, recycled round-robin.
	// Buffer cache: keyed by raw audio file path (not .sfx path).
	// Listener: updated each frame by EditorLayer/GameLayer with camera transform.
	// -------------------------------------------------------------------------
	class AudioEngine
	{
	public:
		static void Init();
		static void Shutdown();

		// Call each frame with the primary camera's world transform for 3D spatialization.
		static void Update(const Vector3& listenerPos, const Vector3& forward, const Vector3& up);

		// Decode & upload raw audio file to an OpenAL buffer (cached per resolved path).
		// Returns 0 if loading fails.
		static ALuint GetOrLoadBuffer(const std::string& rawPath);

		// Play a clip using its baked settings.
		// volumeOverride < 0  → use clip's Volume field.
		// Returns a source handle (non-zero on success) for subsequent control calls.
		static uint32_t Play(const AudioHandle& handle, float volumeOverride = -1.0f);
		static uint32_t PlayAt(const AudioHandle& handle, const Vector3& pos, float volumeOverride = -1.0f);

		static void Stop(uint32_t sourceHandle);
		static void StopAll();
		static void Pause(uint32_t sourceHandle);
		static void Resume(uint32_t sourceHandle);
		static void SetVolume(uint32_t sourceHandle, float volume);
		static void SetMasterVolume(float volume);
		static bool IsPlaying(uint32_t sourceHandle);

		static bool IsInitialized() { return s_Initialized; }

	private:
		static bool s_Initialized;
		static float s_MasterVolume;

		static constexpr int SOURCE_POOL_SIZE = 32;
		static std::array<ALuint, SOURCE_POOL_SIZE> s_Sources;
		static int s_NextSource;

		static std::unordered_map<std::string, ALuint> s_BufferCache;

		// Get/recycle the next available source slot (round-robin).
		static ALuint AcquireSource();
		// Map engine source handle → AL source index (handle = index+1).
		static ALuint SourceFromHandle(uint32_t handle);
	};
}
