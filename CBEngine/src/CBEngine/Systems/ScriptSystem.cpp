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
			auto& script = view.get<ScriptComponent>(entity);

			// Lazy initialization for scripts added at runtime
			if (!script.ScriptLoaded && !script.ScriptPath.empty())
			{
				ScriptEngine::OnEntityCreate(scene, e);
			}

			ScriptEngine::OnEntityUpdate(scene, e, ts);
		}
	}
}
