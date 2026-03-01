#pragma once

#include "ISystem.h"

namespace CB
{
	class WorldSimSystem
	{
	public:
		static void OnUpdate(Scene* scene, Timestep ts);
	};

	class WorldSimSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene, Timestep ts) override { WorldSimSystem::OnUpdate(scene, ts); }
		const char* GetName() const override { return "WorldSimSystem"; }
		int GetPriority() const override { return 155; }
	};
}
