#include "cbpch.h"
#include "ScriptSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Scripting/ScriptEngine.h"

namespace CB
{
	void ScriptSystem::Init(Scene* scene)
	{
		ScriptEngine::Init();

		// Initialize all entities that already have scripts
		auto view = scene->GetRegistry().view<ScriptComponent>();
		for (auto entity : view)
		{
			Entity e{entity, scene};
			ScriptEngine::OnEntityCreate(scene, e);
		}
	}

	void ScriptSystem::Shutdown()
	{
		// ScriptEngine persists across play sessions (global Lua state)
		// Individual script instances are cleaned up per entity
	}

	void ScriptSystem::OnUpdate(Scene* scene, Timestep ts)
	{
		// Update Time table and frame counters for all scripts this frame
		ScriptEngine::BeginFrame(ts);

		auto view = scene->GetRegistry().view<ScriptComponent>();
		for (auto entity : view)
		{
			Entity e{entity, scene};
			auto& scriptComp = view.get<ScriptComponent>(entity);

			// Lazy initialization: check if any script entry needs loading
			bool anyUnloaded = false;
			for (auto& entry : scriptComp.Scripts)
			{
				if (!entry.ScriptLoaded && !entry.ScriptPath.empty())
				{
					anyUnloaded = true;
					break;
				}
			}

			if (anyUnloaded)
			{
				ScriptEngine::OnEntityCreate(scene, e);
			}

			ScriptEngine::OnEntityUpdate(scene, e, ts);
		}

		// Fixed-update loop — fires OnFixedUpdate in step with physics (1/60 s)
		m_FixedAccumulator += ts.GetSeconds();
		int steps = 0;
		while (m_FixedAccumulator >= FIXED_TIMESTEP && steps < MAX_FIXED_STEPS)
		{
			m_FixedAccumulator -= FIXED_TIMESTEP;
			steps++;

			auto fixedView = scene->GetRegistry().view<ScriptComponent>();
			for (auto entity : fixedView)
			{
				Entity e{entity, scene};
				ScriptEngine::OnEntityFixedUpdate(scene, e, Timestep(FIXED_TIMESTEP));
			}
		}
	}

	void ScriptSystem::OnLateUpdate(Scene* scene, Timestep ts)
	{
		auto view = scene->GetRegistry().view<ScriptComponent>();
		for (auto entity : view)
		{
			Entity e{entity, scene};
			ScriptEngine::OnEntityLateUpdate(scene, e, ts);
		}
	}
}
