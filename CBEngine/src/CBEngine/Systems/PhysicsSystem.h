#pragma once

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Core/TimeStep.h"

namespace CB
{
	class PhysicsWorld;

	class PhysicsSystem
	{
	public:
		static void Init(Scene* scene, PhysicsWorld* world);
		static void Shutdown(PhysicsWorld* world);
		static void OnUpdate(Scene* scene, PhysicsWorld* world, Timestep ts);

		// Destroy physics bodies for an entity before it is removed from the registry
		static void DestroyBody(PhysicsWorld* world, entt::entity entity, entt::registry& registry);
		static void DestroyStaticBody(PhysicsWorld* world, entt::entity entity, entt::registry& registry);

	private:
		static void SyncToPhysics(Scene* scene, PhysicsWorld* world);
		static void SyncFromPhysics(Scene* scene, PhysicsWorld* world);
		static void SyncStaticColliders(Scene* scene, PhysicsWorld* world);
		static void CreateBody(Scene* scene, PhysicsWorld* world, entt::entity entity);
		static void CreateStaticColliderBody(Scene* scene, PhysicsWorld* world, entt::entity entity);
	};
}
