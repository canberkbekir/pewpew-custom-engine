#pragma once

#include "ISystem.h"

namespace CB
{
	class ScriptSystem : public ISystem
	{
	public:
		void Init(Scene* scene) override;
		void Shutdown() override;
		void OnUpdate(Scene* scene, Timestep ts) override;
		const char* GetName() const override { return "ScriptSystem"; }
		int GetPriority() const override { return 150; } // After Transform(100), before Destruction(200)
	};
}
