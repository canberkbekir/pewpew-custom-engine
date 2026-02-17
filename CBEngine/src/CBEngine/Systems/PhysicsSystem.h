#pragma once

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Core/TimeStep.h"

namespace CB
{
	class PhysicsWorld;

	class PhysicsSystem
	{
	public:
		static void Init(Scene* scene);
		static void Shutdown();
		static void OnUpdate(Scene* scene, Timestep ts);

	private:
		static void SyncToPhysics(Scene* scene);
		static void SyncFromPhysics(Scene* scene);
		static void CreateBody(Scene* scene, entt::entity entity);
		static void DestroyBody(entt::entity entity);
	};
}
