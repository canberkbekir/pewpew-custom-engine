#pragma once

#include "CBEngine/Math/CoreMath.h"

#include <cstdint>
#include <string>

namespace CB
{
	enum class VoxelMaterialType : uint8_t
	{
		Stone = 0,
		Wood,
		Metal,
		Glass,
		Marble,
		Count
	};

	inline const char* VoxelMaterialTypeToString(VoxelMaterialType type)
	{
		switch (type)
		{
			case VoxelMaterialType::Stone:  return "Stone";
			case VoxelMaterialType::Wood:   return "Wood";
			case VoxelMaterialType::Metal:  return "Metal";
			case VoxelMaterialType::Glass:  return "Glass";
			case VoxelMaterialType::Marble: return "Marble";
			default:                        return "Stone";
		}
	}

	inline VoxelMaterialType VoxelMaterialTypeFromString(const std::string& str)
	{
		if (str == "Wood")   return VoxelMaterialType::Wood;
		if (str == "Metal")  return VoxelMaterialType::Metal;
		if (str == "Glass")  return VoxelMaterialType::Glass;
		if (str == "Marble") return VoxelMaterialType::Marble;
		return VoxelMaterialType::Stone;
	}

	// Returns a display color for each material type (for editor visualization)
	inline Vector3 VoxelMaterialTypeDisplayColor(VoxelMaterialType type)
	{
		switch (type)
		{
			case VoxelMaterialType::Stone:  return { 0.5f, 0.5f, 0.5f };
			case VoxelMaterialType::Wood:   return { 0.55f, 0.35f, 0.15f };
			case VoxelMaterialType::Metal:  return { 0.75f, 0.75f, 0.8f };
			case VoxelMaterialType::Glass:  return { 0.3f, 0.8f, 0.9f };
			case VoxelMaterialType::Marble: return { 0.9f, 0.88f, 0.85f };
			default:                        return { 0.5f, 0.5f, 0.5f };
		}
	}

	// Auto-detect material type from voxel visual properties
	inline VoxelMaterialType AutoDetectMaterialType(const Vector3& color)
	{
		// Luminance for light/dark detection
		float luminance = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;

		// Wood: warm/brown hues, rough
		if (color.r > color.b && color.r > 0.3f
			&& color.g < color.r && color.g > color.b)
			return VoxelMaterialType::Wood;

		// Marble: light color, medium roughness, non-metallic
		if (luminance > 0.7f)
			return VoxelMaterialType::Marble;

		// Default: Stone
		return VoxelMaterialType::Stone;
	}
}
