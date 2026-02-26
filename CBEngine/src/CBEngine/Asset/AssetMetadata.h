#pragma once

#include "CBEngine/Core/UUID.h"
#include "Asset.h"

#include <filesystem>
#include <vector>

namespace CB
{
	struct AssetMetadata
	{
		UUID Handle;
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath; // Relative to assets root

		std::vector<UUID> Dependencies;

		bool IsValid() const { return Handle.IsValid() && Type != AssetType::None; }
	};
}