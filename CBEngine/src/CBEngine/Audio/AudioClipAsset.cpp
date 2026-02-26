#include "cbpch.h"
#include "AudioClipAsset.h"
#include "CBEngine/Core/Log.h"

#include <fstream>
#include <sstream>

namespace CB
{
	// -------------------------------------------------------------------------
	// Parse a simple key=value INI file
	// -------------------------------------------------------------------------
	static std::unordered_map<std::string, std::string> ParseIni(const std::filesystem::path& path)
	{
		std::unordered_map<std::string, std::string> result;
		std::ifstream file(path);
		if (!file.is_open())
			return result;

		std::string line;
		while (std::getline(file, line)) {
			// Strip carriage returns
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			// Skip comments and empty lines
			if (line.empty() || line[0] == '#' || line[0] == ';')
				continue;

			size_t eq = line.find('=');
			if (eq == std::string::npos)
				continue;

			std::string key = line.substr(0, eq);
			std::string value = line.substr(eq + 1);

			// Trim whitespace
			auto trim = [](std::string& s)
			{
				size_t start = s.find_first_not_of(" \t");
				size_t end = s.find_last_not_of(" \t");
				s = (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
			};
			trim(key);
			trim(value);

			result[key] = value;
		}

		return result;
	}

	// -------------------------------------------------------------------------
	// AudioClipAsset::Load
	// -------------------------------------------------------------------------
	Ref<AudioClipAsset> AudioClipAsset::Load(const std::filesystem::path& path)
	{
		auto asset = CreateRef<AudioClipAsset>();
		if (!asset->LoadFromFile(path)) {
			CB_CORE_ERROR("AudioClipAsset: failed to load '{0}'", path.string());
			return nullptr;
		}
		return asset;
	}

	bool AudioClipAsset::LoadFromFile(const std::filesystem::path& path)
	{
		m_Path = path;
		m_Type = AssetType::AudioClip;

		auto ini = ParseIni(path);

		auto get = [&](const std::string& key,const std::string& def) -> std::string
		{
			auto it = ini.find(key);
			return (it != ini.end()) ? it->second : def;
		};

		SourcePath = get("source", "");
		Volume = std::stof(get("volume", "1.0"));
		Pitch = std::stof(get("pitch", "1.0"));
		Loop = (get("loop", "false") == "true");
		SpatialBlend = std::stof(get("spatialBlend", "1.0"));
		MinDistance = std::stof(get("minDistance", "1.0"));
		MaxDistance = std::stof(get("maxDistance", "50.0"));
		Rolloff = get("rolloff", "linear");
		Priority = std::stoi(get("priority", "128"));

		return true;
	}

	bool AudioClipAsset::Reload() { return LoadFromFile(m_Path); }

	bool AudioClipAsset::Save(const std::filesystem::path& path) const
	{
		std::ofstream file(path);
		if (!file.is_open()) {
			CB_CORE_ERROR("AudioClipAsset: cannot save to '{0}'", path.string());
			return false;
		}

		file << "# CBEngine Audio Clip\n";
		file << "source=" << SourcePath << "\n";
		file << "volume=" << Volume << "\n";
		file << "pitch=" << Pitch << "\n";
		file << "loop=" << (Loop ? "true" : "false") << "\n";
		file << "spatialBlend=" << SpatialBlend << "\n";
		file << "minDistance=" << MinDistance << "\n";
		file << "maxDistance=" << MaxDistance << "\n";
		file << "rolloff=" << Rolloff << "\n";
		file << "priority=" << Priority << "\n";

		return true;
	}
}