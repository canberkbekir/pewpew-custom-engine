#pragma once

#include "ISystem.h"

namespace CB
{
	class ScriptSystem : public ISystem
	{
	public:
		void Init(Scene* scene) override;
		void Shutdown() override;
		void OnUpdate(Scene* scene,Timestep ts) override;
		void OnLateUpdate(Scene* scene,Timestep ts) override;
		const char* GetName() const override { return "ScriptSystem"; }
		int GetPriority() const override { return 150; } // After Transform(100), before Destruction(200)

	private:
		// Fixed-update accumulator — fires OnFixedUpdate in sync with PhysicsWorld::FIXED_TIMESTEP
		float m_FixedAccumulator = 0.0f;
		static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
		static constexpr int MAX_FIXED_STEPS = 4; // safety cap to avoid spiral of death
	};
}