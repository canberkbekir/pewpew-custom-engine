#pragma once

#include "ISystem.h"

namespace CB
{
	class WorldRenderSystem
	{
	public:
		static void OnUpdate(Scene* scene, Timestep ts);
	};

	class WorldRenderSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene, Timestep ts) override { WorldRenderSystem::OnUpdate(scene, ts); }
		const char* GetName() const override { return "WorldRenderSystem"; }
		int GetPriority() const override { return 160; }
	};
}
