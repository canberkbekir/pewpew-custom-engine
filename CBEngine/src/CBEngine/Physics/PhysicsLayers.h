#pragma once

#include <cstdint>

namespace CB::PhysicsLayers
{
	static constexpr uint8_t NUM_LAYERS = 16;

	static constexpr uint16_t AllLayers = 0xFFFF;

	static constexpr uint16_t LayerMask(uint8_t layer) { return static_cast<uint16_t>(1 << layer); }

	inline const char* GetLayerName(uint8_t layer)
	{
		static const char* s_Names[NUM_LAYERS] = {
			"Default", // 0
			"Player", // 1
			"Enemy", // 2
			"Environment", // 3
			"Projectile", // 4
			"Trigger", // 5
			"IgnoreRaycast", // 6
			"Custom7", // 7
			"Custom8", // 8
			"Custom9", // 9
			"Custom10", // 10
			"Custom11", // 11
			"Custom12", // 12
			"Custom13", // 13
			"Custom14", // 14
			"Custom15" // 15
		};

		if (layer >= NUM_LAYERS)
			return "Invalid";
		return s_Names[layer];
	}
}