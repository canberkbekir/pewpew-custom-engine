#pragma once

#include "CBEngine/Asset/Asset.h"
#include "CBEngine/Core/Core.h"

#include <string>
#include <filesystem>

// Forward-declare OpenAL type to avoid requiring AL/al.h in header
typedef unsigned int ALuint;

namespace CB
{
	// -------------------------------------------------------------------------
	// AudioHandle — lightweight asset reference for Lua scripts.
	// Mirrors BlueprintHandle pattern: carries only a UUID, never a raw path.
	// -------------------------------------------------------------------------
	struct AudioHandle
	{
		UUID AssetUUID;
		AudioHandle() = default;
		explicit AudioHandle(UUID uuid) : AssetUUID(uuid) {}
		bool IsValid() const { return AssetUUID.IsValid(); }
	};

	// -------------------------------------------------------------------------
	// AudioClipAsset — wraps a .sfx settings file and its decoded audio data.
	// .sfx files use INI-style key=value format:
	//   source=assets/audio/explosion.wav
	//   volume=1.0
	//   pitch=1.0
	//   loop=false
	//   spatialBlend=1.0
	//   minDistance=1.0
	//   maxDistance=50.0
	//   rolloff=linear
	//   priority=128
	// -------------------------------------------------------------------------
	class AudioClipAsset : public Asset
	{
	public:
		// Settings (editable in editor)
		std::string SourcePath;      // Relative path to raw audio file (WAV/MP3)
		float Volume     = 1.0f;
		float Pitch      = 1.0f;
		bool  Loop       = false;
		float SpatialBlend = 1.0f;  // 0 = 2D, 1 = 3D
		float MinDistance = 1.0f;
		float MaxDistance = 50.0f;
		std::string Rolloff = "linear";  // linear, inverseClamped
		int   Priority   = 128;

		// Runtime (not serialized)
		ALuint BufferID  = 0;
		float  Duration  = 0.0f;
		int    SampleRate = 0;

		// Asset interface
		bool Reload() override;
		static AssetType GetStaticType() { return AssetType::AudioClip; }

		// Factory
		static Ref<AudioClipAsset> Load(const std::filesystem::path& path);
		bool Save(const std::filesystem::path& path) const;

	private:
		bool LoadFromFile(const std::filesystem::path& path);
	};
}
