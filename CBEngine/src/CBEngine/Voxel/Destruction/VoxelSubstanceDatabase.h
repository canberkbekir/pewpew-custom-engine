#pragma once

// =========================================================================
// DEPRECATED — Use SubstanceRegistry instead.
// This header is kept as a thin forwarding wrapper during transition.
// =========================================================================

#include "SubstanceRegistry.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

#include <string>

namespace CB
{
	class [[deprecated("Use SubstanceRegistry instead")]] VoxelSubstanceDatabase
	{
	public:
		static void Load(const std::string& path) { SubstanceRegistry::Load(path); }
		static void Save(const std::string& path) { SubstanceRegistry::Save(path); }

		static const VoxelSubstanceProperties& Get(VoxelMaterialType type)
		{
			return SubstanceRegistry::GetByLegacyType(type);
		}

		static const VoxelSubstanceProperties& GetFallback()
		{
			return SubstanceRegistry::GetFallback().Properties;
		}

		static bool IsLoaded() { return SubstanceRegistry::IsLoaded(); }

		static VoxelMaterialType ResolveSubstanceName(const std::string& name)
		{
			return VoxelMaterialTypeFromString(name);
		}

		static VoxelSubstanceProperties& GetMutable(VoxelMaterialType type)
		{
			return SubstanceRegistry::GetMutable(VoxelMaterialTypeToString(type));
		}
	};
}
