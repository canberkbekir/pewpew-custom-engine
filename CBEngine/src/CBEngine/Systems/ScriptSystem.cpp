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
	}
}
