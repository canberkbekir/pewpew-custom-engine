#include "cbpch.h"
#include "AudioEngine.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Asset/AssetManager.h"

// OpenAL headers — guarded so the file compiles even without the vendor library
#ifdef CB_AUDIO_ENABLED
	#include <AL/al.h>
	#include <AL/alc.h>

	// dr_libs — single-header audio decoders
	#define DR_WAV_IMPLEMENTATION
	#include <dr_wav.h>
	#define DR_MP3_IMPLEMENTATION
	#include <dr_mp3.h>
#endif

#include <filesystem>

namespace CB
{
	// -------------------------------------------------------------------------
	// Static member definitions
	// -------------------------------------------------------------------------
	bool                                         AudioEngine::s_Initialized = false;
	float                                        AudioEngine::s_MasterVolume = 1.0f;
	std::array<ALuint, AudioEngine::SOURCE_POOL_SIZE> AudioEngine::s_Sources = {};
	int                                          AudioEngine::s_NextSource   = 0;
	std::unordered_map<std::string, ALuint>      AudioEngine::s_BufferCache;

#ifdef CB_AUDIO_ENABLED
	static ALCdevice*  s_ALDevice  = nullptr;
	static ALCcontext* s_ALContext = nullptr;

	// -------------------------------------------------------------------------
	// Helper: check and log AL error
	// -------------------------------------------------------------------------
	static bool CheckALError(const char* op)
	{
		ALenum err = alGetError();
		if (err != AL_NO_ERROR)
		{
			CB_CORE_ERROR("[AudioEngine] OpenAL error in '{0}': 0x{1:X}", op, err);
			return false;
		}
		return true;
	}
#endif

	// -------------------------------------------------------------------------
	// Init / Shutdown
	// -------------------------------------------------------------------------
	void AudioEngine::Init()
	{
		if (s_Initialized)
			return;

#ifdef CB_AUDIO_ENABLED
		s_ALDevice = alcOpenDevice(nullptr);
		if (!s_ALDevice)
		{
			CB_CORE_ERROR("[AudioEngine] Failed to open OpenAL device");
			return;
		}

		s_ALContext = alcCreateContext(s_ALDevice, nullptr);
		if (!s_ALContext || !alcMakeContextCurrent(s_ALContext))
		{
			CB_CORE_ERROR("[AudioEngine] Failed to create/set OpenAL context");
			if (s_ALContext) alcDestroyContext(s_ALContext);
			alcCloseDevice(s_ALDevice);
			s_ALDevice  = nullptr;
			s_ALContext = nullptr;
			return;
		}

		alGenSources(SOURCE_POOL_SIZE, s_Sources.data());
		CheckALError("alGenSources");

		// Default listener
		alListener3f(AL_POSITION,    0.0f, 0.0f, 0.0f);
		alListener3f(AL_VELOCITY,    0.0f, 0.0f, 0.0f);
		float orientation[6] = { 0.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f };
		alListenerfv(AL_ORIENTATION, orientation);

		s_Initialized = true;
		CB_CORE_INFO("[AudioEngine] Initialized (OpenAL-Soft, {0} sources)", SOURCE_POOL_SIZE);
#else
		CB_CORE_WARN("[AudioEngine] Audio disabled — CB_AUDIO_ENABLED not defined");
#endif
	}

	void AudioEngine::Shutdown()
	{
		if (!s_Initialized)
			return;

#ifdef CB_AUDIO_ENABLED
		StopAll();

		for (auto& [path, bufID] : s_BufferCache)
			alDeleteBuffers(1, &bufID);
		s_BufferCache.clear();

		alDeleteSources(SOURCE_POOL_SIZE, s_Sources.data());
		s_Sources.fill(0);

		alcMakeContextCurrent(nullptr);
		if (s_ALContext) { alcDestroyContext(s_ALContext); s_ALContext = nullptr; }
		if (s_ALDevice)  { alcCloseDevice(s_ALDevice);    s_ALDevice  = nullptr; }
#endif

		s_Initialized = false;
		CB_CORE_INFO("[AudioEngine] Shut down");
	}

	// -------------------------------------------------------------------------
	// Update — listener position for 3D audio
	// -------------------------------------------------------------------------
	void AudioEngine::Update(const Vector3& listenerPos, const Vector3& forward, const Vector3& up)
	{
#ifdef CB_AUDIO_ENABLED
		if (!s_Initialized) return;

		alListener3f(AL_POSITION, listenerPos.x, listenerPos.y, listenerPos.z);

		float orientation[6] = {
			forward.x, forward.y, forward.z,
			up.x,      up.y,      up.z
		};
		alListenerfv(AL_ORIENTATION, orientation);
#endif
	}

	// -------------------------------------------------------------------------
	// GetOrLoadBuffer — decode audio file and cache the AL buffer
	// -------------------------------------------------------------------------
	ALuint AudioEngine::GetOrLoadBuffer(const std::string& rawPath)
	{
		auto it = s_BufferCache.find(rawPath);
		if (it != s_BufferCache.end())
			return it->second;

#ifdef CB_AUDIO_ENABLED
		if (!s_Initialized) return 0;

		// Determine format
		std::string ext = std::filesystem::path(rawPath).extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		ALuint bufferID = 0;
		alGenBuffers(1, &bufferID);

		if (ext == ".wav")
		{
			drwav wav;
			if (!drwav_init_file(&wav, rawPath.c_str(), nullptr))
			{
				CB_CORE_ERROR("[AudioEngine] Failed to open WAV: {0}", rawPath);
				alDeleteBuffers(1, &bufferID);
				return 0;
			}

			std::vector<int16_t> pcm(wav.totalPCMFrameCount * wav.channels);
			drwav_uint64 framesRead = drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, pcm.data());

			ALenum format = (wav.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
			alBufferData(bufferID, format, pcm.data(),
				static_cast<ALsizei>(framesRead * wav.channels * sizeof(int16_t)),
				static_cast<ALsizei>(wav.sampleRate));
			drwav_uninit(&wav);
		}
		else if (ext == ".mp3")
		{
			drmp3_config cfg{};
			drmp3_uint64 frameCount = 0;
			int16_t* data = drmp3_open_file_and_read_pcm_frames_s16(rawPath.c_str(), &cfg, &frameCount, nullptr);
			if (!data)
			{
				CB_CORE_ERROR("[AudioEngine] Failed to open MP3: {0}", rawPath);
				alDeleteBuffers(1, &bufferID);
				return 0;
			}

			ALenum format = (cfg.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
			alBufferData(bufferID, format, data,
				static_cast<ALsizei>(frameCount * cfg.channels * sizeof(int16_t)),
				static_cast<ALsizei>(cfg.sampleRate));
			drmp3_free(data, nullptr);
		}
		else
		{
			CB_CORE_ERROR("[AudioEngine] Unsupported audio format: {0}", ext);
			alDeleteBuffers(1, &bufferID);
			return 0;
		}

		if (!CheckALError("alBufferData"))
		{
			alDeleteBuffers(1, &bufferID);
			return 0;
		}

		s_BufferCache[rawPath] = bufferID;
		return bufferID;
#else
		return 0;
#endif
	}

	// -------------------------------------------------------------------------
	// Internal helpers
	// -------------------------------------------------------------------------
	ALuint AudioEngine::AcquireSource()
	{
		// Round-robin; stop current playback on recycled source
		ALuint src = s_Sources[s_NextSource];
		s_NextSource = (s_NextSource + 1) % SOURCE_POOL_SIZE;
#ifdef CB_AUDIO_ENABLED
		alSourceStop(src);
#endif
		return src;
	}

	ALuint AudioEngine::SourceFromHandle(uint32_t handle)
	{
		if (handle == 0 || handle > SOURCE_POOL_SIZE) return 0;
		return s_Sources[handle - 1];
	}

	// -------------------------------------------------------------------------
	// Play / PlayAt
	// -------------------------------------------------------------------------
	uint32_t AudioEngine::Play(const AudioHandle& handle, float volumeOverride)
	{
#ifdef CB_AUDIO_ENABLED
		if (!s_Initialized || !handle.IsValid()) return 0;

		auto clip = AssetManager::GetAsset<AudioClipAsset>(handle.AssetUUID);
		if (!clip) return 0;

		// Resolve source audio path relative to asset directory
		std::string rawPath = clip->SourcePath;
		if (!std::filesystem::exists(rawPath))
		{
			auto resolved = AssetManager::GetAssetDirectory() / clip->SourcePath;
			rawPath = resolved.string();
		}

		ALuint buf = GetOrLoadBuffer(rawPath);
		if (!buf) return 0;

		ALuint src = AcquireSource();
		int slot = s_NextSource == 0 ? SOURCE_POOL_SIZE - 1 : s_NextSource - 1;

		float vol = (volumeOverride >= 0.0f ? volumeOverride : clip->Volume) * s_MasterVolume;
		alSourcei (src, AL_BUFFER,    static_cast<ALint>(buf));
		alSourcef (src, AL_GAIN,      vol);
		alSourcef (src, AL_PITCH,     clip->Pitch);
		alSourcei (src, AL_LOOPING,   clip->Loop ? AL_TRUE : AL_FALSE);

		// 2D: disable distance attenuation
		alSourcei (src, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(src, AL_POSITION,  0.0f, 0.0f, 0.0f);

		alSourcePlay(src);
		CheckALError("alSourcePlay");

		return static_cast<uint32_t>(slot + 1);
#else
		return 0;
#endif
	}

	uint32_t AudioEngine::PlayAt(const AudioHandle& handle, const Vector3& pos, float volumeOverride)
	{
#ifdef CB_AUDIO_ENABLED
		if (!s_Initialized || !handle.IsValid()) return 0;

		auto clip = AssetManager::GetAsset<AudioClipAsset>(handle.AssetUUID);
		if (!clip) return 0;

		std::string rawPath = clip->SourcePath;
		if (!std::filesystem::exists(rawPath))
		{
			auto resolved = AssetManager::GetAssetDirectory() / clip->SourcePath;
			rawPath = resolved.string();
		}

		ALuint buf = GetOrLoadBuffer(rawPath);
		if (!buf) return 0;

		ALuint src = AcquireSource();
		int slot = s_NextSource == 0 ? SOURCE_POOL_SIZE - 1 : s_NextSource - 1;

		float vol = (volumeOverride >= 0.0f ? volumeOverride : clip->Volume) * s_MasterVolume;
		float blend = clip->SpatialBlend;

		alSourcei (src, AL_BUFFER,    static_cast<ALint>(buf));
		alSourcef (src, AL_GAIN,      vol);
		alSourcef (src, AL_PITCH,     clip->Pitch);
		alSourcei (src, AL_LOOPING,   clip->Loop ? AL_TRUE : AL_FALSE);
		alSourcef (src, AL_REFERENCE_DISTANCE, clip->MinDistance);
		alSourcef (src, AL_MAX_DISTANCE,       clip->MaxDistance);

		if (blend > 0.0f)
		{
			alSourcei (src, AL_SOURCE_RELATIVE, AL_FALSE);
			alSource3f(src, AL_POSITION, pos.x, pos.y, pos.z);
		}
		else
		{
			alSourcei (src, AL_SOURCE_RELATIVE, AL_TRUE);
			alSource3f(src, AL_POSITION, 0.0f, 0.0f, 0.0f);
		}

		alSourcePlay(src);
		CheckALError("alSourcePlay (3D)");

		return static_cast<uint32_t>(slot + 1);
#else
		return 0;
#endif
	}

	// -------------------------------------------------------------------------
	// Control
	// -------------------------------------------------------------------------
	void AudioEngine::Stop(uint32_t sourceHandle)
	{
#ifdef CB_AUDIO_ENABLED
		ALuint src = SourceFromHandle(sourceHandle);
		if (src) alSourceStop(src);
#endif
	}

	void AudioEngine::StopAll()
	{
#ifdef CB_AUDIO_ENABLED
		if (!s_Initialized) return;
		for (ALuint src : s_Sources)
			if (src) alSourceStop(src);
#endif
	}

	void AudioEngine::Pause(uint32_t sourceHandle)
	{
#ifdef CB_AUDIO_ENABLED
		ALuint src = SourceFromHandle(sourceHandle);
		if (src) alSourcePause(src);
#endif
	}

	void AudioEngine::Resume(uint32_t sourceHandle)
	{
#ifdef CB_AUDIO_ENABLED
		ALuint src = SourceFromHandle(sourceHandle);
		if (src) alSourcePlay(src);
#endif
	}

	void AudioEngine::SetVolume(uint32_t sourceHandle, float volume)
	{
#ifdef CB_AUDIO_ENABLED
		ALuint src = SourceFromHandle(sourceHandle);
		if (src) alSourcef(src, AL_GAIN, volume * s_MasterVolume);
#endif
	}

	void AudioEngine::SetMasterVolume(float volume)
	{
		s_MasterVolume = glm::clamp(volume, 0.0f, 1.0f);
#ifdef CB_AUDIO_ENABLED
		alListenerf(AL_GAIN, s_MasterVolume);
#endif
	}

	bool AudioEngine::IsPlaying(uint32_t sourceHandle)
	{
#ifdef CB_AUDIO_ENABLED
		ALuint src = SourceFromHandle(sourceHandle);
		if (!src) return false;
		ALint state = AL_STOPPED;
		alGetSourcei(src, AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
#else
		return false;
#endif
	}
}
