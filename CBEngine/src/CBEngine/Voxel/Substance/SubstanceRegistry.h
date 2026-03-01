#pragma once

#include "SubstanceID.h"
#include "SubstanceDefinition.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace CB
{
	class SubstanceRegistry
	{
	public:
		static void Load(const std::string& path);
		static void Save(const std::string& path);

		static const VoxelSubstanceProperties& Get(const SubstanceID& id);
		static SubstanceDefinition& GetDefinition(const SubstanceID& id);
		static const SubstanceDefinition& GetFallback();
		static std::vector<SubstanceID> GetAllIDs();
		static bool Exists(const SubstanceID& id);

		// Returns the first registered substance ID, or empty string if none loaded
		static SubstanceID GetFirstID()
		{
			auto ids = GetAllIDs();
			return ids.empty() ? SubstanceID{} : ids[0];
		}

		// Returns id if it exists in the registry, otherwise returns the first available ID
		static SubstanceID ResolveID(const SubstanceID& id)
		{
			if (!id.empty() && Exists(id)) return id;
			return GetFirstID();
		}

		static void Register(SubstanceDefinition def);
		static void Remove(const SubstanceID& id);
		static void Duplicate(const SubstanceID& sourceID, const SubstanceID& newID);
		static void Rename(const SubstanceID& oldID, const SubstanceID& newID);

		// Mutable access for editor
		static VoxelSubstanceProperties& GetMutable(const SubstanceID& id);

		// Legacy bridge: map old enum to string key
		static const VoxelSubstanceProperties& GetByLegacyType(VoxelMaterialType type);
		static Vector3 GetDisplayColor(const SubstanceID& id);

		static bool IsLoaded() { return s_Loaded; }

	private:
		static std::unordered_map<SubstanceID, SubstanceDefinition> s_Substances;
		static SubstanceDefinition s_Fallback;
		static bool s_Loaded;
		static std::string s_LoadedPath;
	};
}
