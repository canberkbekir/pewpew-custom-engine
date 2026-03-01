#include "cbpch.h"
#include "Scene.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Systems/ISystem.h"
#include "CBEngine/Systems/TransformSystem.h"
#include "CBEngine/Systems/PhysicsSystem.h"
#include "CBEngine/Systems/DestructionSystem.h"
#include "CBEngine/Systems/ScriptSystem.h"
#include "CBEngine/Systems/HingeJointSystem.h"
#include "CBEngine/Systems/HingeChainSystem.h"
#include "CBEngine/Systems/VoxelDestructionSystem.h"
#include "CBEngine/Systems/VoxelSpreadSystem.h"
#include "CBEngine/Systems/WorldRenderSystem.h"
#include "CBEngine/Systems/WorldSimSystem.h"
#include "CBEngine/Voxel/World/WorldGrid.h"
#include "CBEngine/Voxel/World/CellularAutomata.h"
#include "CBEngine/Components/HingeJointComponent.h"
#include "CBEngine/Components/HingeChainComponent.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Components/WorldGridComponent.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Events/SceneEvent.h"
#include "CBEngine/Input/Input.h"
#include "CBEngine/Debug/Instrumentor.h"

namespace CB
{
	// ---- Built-in system adapters ----

	class TransformSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene,Timestep ts) override { TransformSystem::OnUpdate(scene, ts); }
		const char* GetName() const override { return "TransformSystem"; }
		int GetPriority() const override { return 100; }
	};

	class DestructionSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene,Timestep ts) override { DestructionSystem::OnUpdate(scene, ts); }
		const char* GetName() const override { return "DestructionSystem"; }
		int GetPriority() const override { return 200; }
	};

	class PhysicsSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene,Timestep ts) override
		{
			PhysicsSystem::OnUpdate(scene, scene->GetPhysicsWorld(), ts);
		}
		const char* GetName() const override { return "PhysicsSystem"; }
		int GetPriority() const override { return 300; }
	};

	class TransformPostPhysicsAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene,Timestep ts) override { TransformSystem::OnUpdate(scene, ts); }
		const char* GetName() const override { return "TransformPostPhysics"; }
		int GetPriority() const override { return 400; }
	};

	class HingeJointSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene,Timestep ts) override { HingeJointSystem::OnUpdate(scene, ts); }
		const char* GetName() const override { return "HingeJointSystem"; }
		int GetPriority() const override { return 350; }
	};

	// ---- System registration ----

	void Scene::RegisterSystem(Scope<ISystem> system)
	{
		int priority = system->GetPriority();
		auto it = std::lower_bound(m_Systems.begin(), m_Systems.end(), priority,
		                           [](const Scope<ISystem>& s,int p) { return s->GetPriority() < p; });
		m_Systems.insert(it, std::move(system));
	}

	void Scene::UnregisterSystem(const char* name)
	{
		auto it = std::remove_if(m_Systems.begin(), m_Systems.end(),
			[name](const Scope<ISystem>& s)
			{
				if (std::strcmp(s->GetName(), name) == 0)
				{
					s->Shutdown();
					return true;
				}
				return false;
			});
		m_Systems.erase(it, m_Systems.end());
	}

	// ---- Component lifecycle signals ----
	// Free function: retrieves Scene* from registry context, calls DestroyChainLinks.
	// Connected once in Scene::Scene() via on_destroy<HingeChainComponent>.
	// This is the generic hook point — future composite components follow the same pattern.
	static void OnHingeChainComponentDestroyed(entt::registry& reg,entt::entity e)
	{
		Scene** scenePtr = reg.ctx().find<Scene*>();
		if (scenePtr && *scenePtr)
			HingeChainSystem::DestroyChainLinks(*scenePtr, e, reg);
	}

	static void OnDestructibleVoxelComponentDestroyed(entt::registry& reg,entt::entity e)
	{
		if (reg.valid(e) && reg.all_of<IDComponent>(e)) {
			uint64_t uuid = reg.get<IDComponent>(e).ID;
			VoxelDestructionSystem::OnEntityDestroyed(uuid);
		}
	}

	// ---- Scene lifecycle ----

	Scene::Scene()
	{
		// Store this Scene* in the registry context so signal callbacks can retrieve it
		// without coupling to a specific Scene instance.
		m_Registry.ctx().insert_or_assign<Scene*>(this);

		// Wire on_destroy signal for HingeChainComponent — cleanup runs automatically
		// when the component is removed or the entity is destroyed.
		m_Registry.on_destroy<HingeChainComponent>()
		          .connect<&OnHingeChainComponentDestroyed>();

		m_Registry.on_destroy<DestructibleVoxelComponent>()
		          .connect<&OnDestructibleVoxelComponentDestroyed>();

		// Register always-active systems (edit mode + play mode)
		RegisterSystem(CreateScope<TransformSystemAdapter>());    // priority 100
		RegisterSystem(CreateScope<WorldRenderSystemAdapter>()); // priority 160 — mesh building + streaming
	}

	Scene::~Scene()
	{
		if (m_PhysicsInitialized)
			ShutdownPhysics();

		// Destroy WorldGrid (end of scene lifetime)
		{
			auto** wg = m_Registry.ctx().find<WorldGrid*>();
			if (wg && *wg)
			{
				delete *wg;
				m_Registry.ctx().erase<WorldGrid*>();
			}
		}

		// Shutdown all systems
		for (auto& system : m_Systems)
			system->Shutdown();
		m_Systems.clear();
	}

	Entity Scene::CreateEntity(const String Name) { return CreateEntityWithUUID(UUID(), Name); }

	Entity Scene::CreateEntityWithUUID(const UUID UUID,const String Name)
	{
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(UUID);
		String entityName = Name.empty() ? "Entity" : Name;
		entity.AddComponent<TagComponent>(entityName);
		entity.AddComponent<TransformComponent>();

		m_EntityMap[UUID] = entity;

		EntityCreatedEvent event(UUID, entityName);
		if (m_EventCallback)
			m_EventCallback(event);

		return entity;
	}

	void Scene::DestroyEntity(const Entity EntityToDelete)
	{
		// Clean up hierarchy before destroying
		if (EntityToDelete.HasComponent<TransformComponent>()) {
			auto& transform = EntityToDelete.GetComponent<TransformComponent>();
			UUID myUUID = EntityToDelete.GetUUID();

			// Remove from parent's children list
			if (transform.HasParent()) {
				Entity parent = GetEntityByUUID(transform.Parent);
				if (parent) {
					auto& parentChildren = parent.GetComponent<TransformComponent>().Children;
					parentChildren.erase(
						std::remove(parentChildren.begin(), parentChildren.end(), myUUID),
						parentChildren.end()
					);
				}
			}

			// Recursively destroy all children
			auto children = transform.Children; // copy, since DestroyEntity modifies the list
			for (UUID childUUID : children) {
				Entity child = GetEntityByUUID(childUUID);
				if (child)
					DestroyEntity(child);
			}
		}

		// Destroy physics bodies before removing entity from registry
		if (m_PhysicsWorld) {
			HingeJointSystem::DestroyConstraint(m_PhysicsWorld.get(), EntityToDelete, m_Registry);
			PhysicsSystem::DestroyBody(m_PhysicsWorld.get(), EntityToDelete, m_Registry);
			PhysicsSystem::DestroyStaticBody(m_PhysicsWorld.get(), EntityToDelete, m_Registry);
		}

		m_EntityMap.erase(EntityToDelete.GetUUID());
		m_Registry.destroy(EntityToDelete);
	}

	Entity Scene::GetEntityByUUID(const UUID UUID)
	{
		if (m_EntityMap.find(UUID) != m_EntityMap.end())
			return {m_EntityMap.at(UUID), this};
		return {};
	}

	bool Scene::EntityExists(const UUID UUID) { return m_EntityMap.find(UUID) != m_EntityMap.end(); }

	void Scene::InitPhysics()
	{
		if (m_PhysicsInitialized)
			return;

		// Ensure all world matrices are up-to-date before physics reads positions
		TransformSystem::OnUpdate(this, Timestep(0.0f));

		// Snapshot scene state before play so we can restore on stop
		{
			Ref<Scene> self(this, [](Scene*)
			{
			}); // non-owning shared_ptr
			SceneSerializer serializer(self);
			m_PrePlaySnapshot = serializer.SerializeToString();
		}

		// Create PhysicsWorld owned by the Scene
		m_PhysicsWorld = CreateScope<PhysicsWorld>();
		m_PhysicsWorld->Init();

		PhysicsSystem::Init(this, m_PhysicsWorld.get());

		// Register script system (runs after transform, before destruction)
		auto scriptSystem = CreateScope<ScriptSystem>();
		scriptSystem->Init(this);
		RegisterSystem(std::move(scriptSystem));

		// Reuse existing WorldGrid (from editor Build) or create a new one
		{
			auto** existingWG = m_Registry.ctx().find<WorldGrid*>();
			WorldGrid* worldGrid;
			if (existingWG && *existingWG)
			{
				worldGrid = *existingWG;
			}
			else
			{
				worldGrid = new WorldGrid();
				WorldGrid* wgPtr = worldGrid;
				m_Registry.ctx().insert_or_assign<WorldGrid*>(std::move(wgPtr));
			}

			// Apply component settings if present
			auto wgcView = m_Registry.view<WorldGridComponent>();
			if (!wgcView.empty())
				wgcView.get<WorldGridComponent>(*wgcView.begin()).ApplyToGrid(*worldGrid);

			worldGrid->BuildSubstanceLUT();
			worldGrid->BuildGlobalPalette();
			CellularAutomata::Init(*worldGrid);

			// Mark all filled chunks as physics-dirty so bodies are created for
			// terrain that was built in edit mode (where PhysicsWorld didn't exist)
			for (auto& [coord, chunk] : worldGrid->Chunks())
			{
				if (chunk && chunk->FilledCount > 0 && !chunk->HasPhysicsBody)
					chunk->PhysicsDirty = true;
			}
		}

		// Register play-mode-only world sim system
		RegisterSystem(CreateScope<WorldSimSystemAdapter>());    // priority 155
		// WorldRenderSystemAdapter is registered in Scene() constructor (always active)

		// Register physics-phase systems
		RegisterSystem(CreateScope<VoxelDestructionSystemAdapter>()); // priority 175
		RegisterSystem(CreateScope<VoxelSpreadSystemAdapter>()); // priority 176
		RegisterSystem(CreateScope<DestructionSystemAdapter>());
		RegisterSystem(CreateScope<PhysicsSystemAdapter>());
		RegisterSystem(CreateScope<HingeJointSystemAdapter>());
		RegisterSystem(CreateScope<TransformPostPhysicsAdapter>());

		m_PhysicsInitialized = true;
		CB_CORE_INFO("Play mode started (scene snapshot saved)");
	}

	void Scene::ShutdownPhysics()
	{
		if (!m_PhysicsInitialized)
			return;

		// Always release cursor lock when stopping play mode
		Input::SetCursorLocked(false);

		// Unregister physics-phase and script systems
		UnregisterSystem("TransformPostPhysics");
		UnregisterSystem("HingeJointSystem");
		UnregisterSystem("PhysicsSystem");
		UnregisterSystem("DestructionSystem");
		UnregisterSystem("VoxelDestructionSystem");
		UnregisterSystem("VoxelSpreadSystem");
		// WorldRenderSystem is always-active (registered in constructor), not unregistered here
		UnregisterSystem("WorldSimSystem");
		UnregisterSystem("ScriptSystem");

		// Destroy all hinge joint constraints before physics world shutdown
		{
			auto view = m_Registry.view<HingeJointComponent>();
			for (auto e : view)
				HingeJointSystem::DestroyConstraint(m_PhysicsWorld.get(), e, m_Registry);
		}

		// Destroy chunk physics bodies (they require PhysicsWorld which is play-mode-only)
		// but keep the WorldGrid itself alive so terrain persists across play/stop.
		// Mark chunks as PhysicsDirty so bodies are recreated on next play.
		{
			auto** wg = m_Registry.ctx().find<WorldGrid*>();
			if (wg && *wg) {
				WorldGrid& grid = **wg;
				for (auto& [coord, chunk] : grid.Chunks()) {
					if (chunk && chunk->HasPhysicsBody) {
						auto& bodyInterface = m_PhysicsWorld->GetBodyInterface();
						bodyInterface.RemoveBody(chunk->PhysicsBodyID);
						bodyInterface.DestroyBody(chunk->PhysicsBodyID);
						chunk->HasPhysicsBody = false;
						chunk->PhysicsDirty = true;
					}
				}
			}
		}

		PhysicsSystem::Shutdown(m_PhysicsWorld.get());
		m_PhysicsWorld.reset();
		m_PhysicsInitialized = false;
		m_PhysicsPaused = false;
		m_PhysicsStepOneFrame = false;

		// Restore scene state from pre-play snapshot
		if (!m_PrePlaySnapshot.empty()) {
			// Suppress the on_destroy<HingeChainComponent> signal during mass-clear.
			// The signal calls scene->DestroyEntity on tracked link children, but the
			// mass-clear below uses registry.destroy() directly — mixing the two on the
			// same entity handles causes double-destroy UB.  Clearing the UUID lists
			// makes the signal a safe no-op for entities in the toDestroy set.
			{
				auto chainView = m_Registry.view<HingeChainComponent>();
				chainView.each([](HingeChainComponent& chain) { chain.LinkEntityUUIDs.clear(); });
			}

			// Suppress on_destroy<DestructibleVoxelComponent> during mass-clear.
			// VoxelDestructionSystem::Shutdown() already clears all state.
			VoxelDestructionSystem::Shutdown();

			// Clear all current entities
			std::vector<entt::entity> toDestroy;
			m_Registry.view<IDComponent>().each([&](entt::entity e,IDComponent&) { toDestroy.push_back(e); });
			for (auto e : toDestroy) {
				auto& id = m_Registry.get<IDComponent>(e);
				m_EntityMap.erase(id.ID);
				m_Registry.destroy(e);
			}
			m_DeferredDestroys.clear();

			// Deserialize from snapshot
			{
				Ref<Scene> self(this, [](Scene*)
				{
				}); // non-owning shared_ptr
				SceneSerializer serializer(self);
				serializer.DeserializeFromString(m_PrePlaySnapshot);
			}
			m_PrePlaySnapshot.clear();
			CB_CORE_INFO("Play mode stopped (scene restored from snapshot)");
		}
	}

	void Scene::OnUpdate(Timestep ts)
	{
		CB_PROFILE_SCOPE_CAT("Scene::OnUpdate", "Scene");
		// 1. Process deferred destruction
		ProcessDeferredDestroys();

		// 2. Run all registered systems in priority order
		//    Pre-physics transform (100), then if physics active:
		//    destruction (200), physics (300), post-physics transform (400)
		for (auto& system : m_Systems) {
			// Physics-phase systems (priority >= 200) only run when physics is active
			if (system->GetPriority() >= 200) {
				if (!m_PhysicsInitialized)
					continue;
				if (m_PhysicsPaused && !m_PhysicsStepOneFrame)
					continue;
			}

			system->OnUpdate(this, ts);
		}

		if (m_PhysicsStepOneFrame)
			m_PhysicsStepOneFrame = false;

		// 3. Late update pass — runs after all systems (e.g. camera follow after physics)
		for (auto& system : m_Systems) {
			if (system->GetPriority() >= 200) {
				if (!m_PhysicsInitialized)
					continue;
				if (m_PhysicsPaused)
					continue;
			}

			system->OnLateUpdate(this, ts);
		}
	}

	void Scene::OnRender()
	{
		// Rendering is handled by EditorLayer/ViewportPanel which calls RenderSystem
	}

	void Scene::ProcessDeferredDestroys()
	{
		CB_PROFILE_SCOPE_CAT("Scene::ProcessDeferredDestroys", "Scene");
		for (auto entity : m_DeferredDestroys) {
			// Destroy physics bodies before removing entity from registry
			if (m_PhysicsWorld) {
				HingeJointSystem::DestroyConstraint(m_PhysicsWorld.get(), entity, m_Registry);
				PhysicsSystem::DestroyBody(m_PhysicsWorld.get(), entity, m_Registry);
				PhysicsSystem::DestroyStaticBody(m_PhysicsWorld.get(), entity, m_Registry);
			}

			auto& id = m_Registry.get<IDComponent>(entity);
			m_EntityMap.erase(id.ID);
			m_Registry.destroy(entity);
		}
		m_DeferredDestroys.clear();
	}
}