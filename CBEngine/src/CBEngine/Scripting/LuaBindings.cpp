#include "cbpch.h"
#include "LuaBindings.h"
#include "ComponentProxies.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Components/CameraComponent.h"
#include "CBEngine/Input/Input.h"
#include "CBEngine/Input/KeyCodes.h"
#include "CBEngine/Input/MouseButtonCodes.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Physics/PhysicsLayers.h"
#include "CBEngine/Debug/DebugDraw.h"

#include <Jolt/Physics/Body/BodyInterface.h>

namespace CB
{

	void LuaBindings::RegisterAll(sol::state& lua)
	{
		RegisterMath(lua);
		RegisterInput(lua);
		RegisterEntity(lua);
		RegisterScene(lua);
		RegisterLog(lua);
		RegisterComponents(lua);
		RegisterPhysics(lua);
		RegisterDebug(lua);
		RegisterFieldTypes(lua);
	}

	void LuaBindings::RegisterMath(sol::state& lua)
	{
		// Vec3 type
		lua.new_usertype<Vector3>("Vec3",
			sol::constructors<Vector3(), Vector3(float), Vector3(float, float, float)>(),
			sol::call_constructor, sol::constructors<Vector3(), Vector3(float), Vector3(float, float, float)>(),
			"x", &Vector3::x,
			"y", &Vector3::y,
			"z", &Vector3::z,
			sol::meta_function::addition, [](const Vector3& a, const Vector3& b) { return a + b; },
			sol::meta_function::subtraction, [](const Vector3& a, const Vector3& b) { return a - b; },
			sol::meta_function::multiplication, sol::overload(
				[](const Vector3& a, float s) { return a * s; },
				[](float s, const Vector3& a) { return s * a; }
			),
			"Length", [](const Vector3& v) { return glm::length(v); },
			"Normalized", [](const Vector3& v) { return glm::normalize(v); },
			"Dot", [](const Vector3& a, const Vector3& b) { return glm::dot(a, b); },
			"Cross", [](const Vector3& a, const Vector3& b) { return glm::cross(a, b); }
		);

		// Quat type (simplified)
		lua.new_usertype<glm::quat>("Quat",
			sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
			sol::call_constructor, sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
			"w", &glm::quat::w,
			"x", &glm::quat::x,
			"y", &glm::quat::y,
			"z", &glm::quat::z,
			"EulerAngles", [](const glm::quat& q) { return glm::eulerAngles(q); }
		);
	}

	void LuaBindings::RegisterInput(sol::state& lua)
	{
		// Input table
		auto input = lua.create_named_table("Input");
		input["IsKeyPressed"] = [](int keyCode) { return Input::IsKeyPressed(keyCode); };
		input["IsMouseButtonPressed"] = [](int button) { return Input::IsMouseButtonPressed(button); };
		input["GetMousePosition"] = []() -> std::pair<float, float> {
			auto [x, y] = Input::GetMousePosition();
			return {x, y};
		};
		input["GetMouseX"] = []() { return Input::GetMouseX(); };
		input["GetMouseY"] = []() { return Input::GetMouseY(); };

		// Key constants
		auto key = lua.create_named_table("Key");
		key["Space"] = CB_KEY_SPACE;
		key["Escape"] = CB_KEY_ESCAPE;
		key["Enter"] = CB_KEY_ENTER;
		key["Tab"] = CB_KEY_TAB;
		key["W"] = CB_KEY_W;
		key["A"] = CB_KEY_A;
		key["S"] = CB_KEY_S;
		key["D"] = CB_KEY_D;
		key["Q"] = CB_KEY_Q;
		key["E"] = CB_KEY_E;
		key["R"] = CB_KEY_R;
		key["F"] = CB_KEY_F;
		key["Up"] = CB_KEY_UP;
		key["Down"] = CB_KEY_DOWN;
		key["Left"] = CB_KEY_LEFT;
		key["Right"] = CB_KEY_RIGHT;
		key["LeftShift"] = CB_KEY_LEFT_SHIFT;
		key["LeftControl"] = CB_KEY_LEFT_CONTROL;
		key["LeftAlt"] = CB_KEY_LEFT_ALT;

		// Mouse button constants
		auto mouse = lua.create_named_table("Mouse");
		mouse["Left"] = CB_MOUSE_BUTTON_LEFT;
		mouse["Right"] = CB_MOUSE_BUTTON_RIGHT;
		mouse["Middle"] = CB_MOUSE_BUTTON_MIDDLE;
	}

	void LuaBindings::RegisterEntity(sol::state& lua)
	{
		lua.new_usertype<Entity>("Entity",
			// Position
			"GetPosition", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().Position;
				return Vector3(0.0f);
			},
			"SetPosition", [](Entity& e, const Vector3& pos) {
				if (e.HasComponent<TransformComponent>())
				{
					e.GetComponent<TransformComponent>().Position = pos;
					e.GetComponent<TransformComponent>().Dirty = true;
				}
			},
			"GetWorldPosition", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().GetWorldPosition();
				return Vector3(0.0f);
			},

			// Rotation (euler angles)
			"GetRotation", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().Rotation;
				return Vector3(0.0f);
			},
			"SetRotation", [](Entity& e, const Vector3& rot) {
				if (e.HasComponent<TransformComponent>())
				{
					e.GetComponent<TransformComponent>().Rotation = rot;
					e.GetComponent<TransformComponent>().Dirty = true;
				}
			},

			// Scale
			"GetScale", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().Scale;
				return Vector3(1.0f);
			},
			"SetScale", [](Entity& e, const Vector3& scale) {
				if (e.HasComponent<TransformComponent>())
				{
					e.GetComponent<TransformComponent>().Scale = scale;
					e.GetComponent<TransformComponent>().Dirty = true;
				}
			},

			// Direction vectors
			"GetForward", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().GetForward();
				return Vector3(0.0f, 0.0f, 1.0f);
			},
			"GetRight", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().GetRight();
				return Vector3(1.0f, 0.0f, 0.0f);
			},
			"GetUp", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().GetUp();
				return Vector3(0.0f, 1.0f, 0.0f);
			},

			// Identity
			"GetName", [](Entity& e) -> std::string {
				return std::string(e.GetName());
			},
			"SetName", [](Entity& e, const std::string& name) {
				if (e.HasComponent<TagComponent>())
					e.GetComponent<TagComponent>().Tag = name;
			},
			"GetUUID", [](Entity& e) -> uint64_t {
				return static_cast<uint64_t>(e.GetUUID());
			},
			"IsValid", [](Entity& e) -> bool { return static_cast<bool>(e); },

			// --- Hierarchy ---
			"SetParent", [](Entity& e, Entity parent, sol::optional<bool> keepWorld) {
				e.SetParent(parent, keepWorld.value_or(true));
			},
			"RemoveParent", [](Entity& e, sol::optional<bool> keepWorld) {
				e.RemoveParent(keepWorld.value_or(true));
			},
			"GetParent", [](Entity& e) -> Entity {
				return e.GetParent();
			},
			"GetChildren", [](Entity& e, sol::this_state L) -> sol::table {
				sol::state_view lua(L);
				sol::table result = lua.create_table();
				auto children = e.GetChildren();
				for (size_t i = 0; i < children.size(); i++)
					result[i + 1] = children[i];
				return result;
			},
			"HasParent", [](Entity& e) -> bool { return e.HasParent(); },
			"HasChildren", [](Entity& e) -> bool { return e.HasChildren(); },
			"IsDescendantOf", [](Entity& e, Entity ancestor) -> bool {
				return e.IsDescendantOf(ancestor);
			},

			// --- Visibility ---
			"SetVisible", [](Entity& e, bool visible) {
				if (e.HasComponent<MeshRendererComponent>())
					e.GetComponent<MeshRendererComponent>().Visible = visible;
				if (e.HasComponent<VoxelRendererComponent>())
					e.GetComponent<VoxelRendererComponent>().Visible = visible;
			},
			"IsVisible", [](Entity& e) -> bool {
				if (e.HasComponent<MeshRendererComponent>())
					return e.GetComponent<MeshRendererComponent>().Visible;
				if (e.HasComponent<VoxelRendererComponent>())
					return e.GetComponent<VoxelRendererComponent>().Visible;
				return true;
			}
		);
	}

	void LuaBindings::RegisterScene(sol::state& lua)
	{
		// Scene is accessed through self._scene
		lua.new_usertype<Scene>("Scene",
			"FindEntity", [](Scene& scene, const std::string& name) -> Entity {
				// Linear search by name
				auto view = scene.GetRegistry().view<TagComponent>();
				for (auto entity : view)
				{
					auto& tag = view.get<TagComponent>(entity);
					if (tag.Tag == name)
						return Entity{entity, &scene};
				}
				return Entity{};
			},
			"CreateEntity", [](Scene& scene, const std::string& name) -> Entity {
				return scene.CreateEntity(name);
			},
			"DestroyEntity", [](Scene& scene, Entity entity) {
				if (entity)
					scene.DestroyEntity(entity);
			},
			"GetEntityByUUID", [](Scene& scene, uint64_t uuid) -> Entity {
				return scene.GetEntityByUUID(UUID(uuid));
			},
			"EntityExists", [](Scene& scene, uint64_t uuid) -> bool {
				return scene.EntityExists(UUID(uuid));
			}
		);
	}

	void LuaBindings::RegisterLog(sol::state& lua)
	{
		auto log = lua.create_named_table("Log");
		log["Info"] = [](const std::string& msg) { CB_CORE_INFO("[Lua] {0}", msg); };
		log["Warn"] = [](const std::string& msg) { CB_CORE_WARN("[Lua] {0}", msg); };
		log["Error"] = [](const std::string& msg) { CB_CORE_ERROR("[Lua] {0}", msg); };
		log["Trace"] = [](const std::string& msg) { CB_CORE_TRACE("[Lua] {0}", msg); };

		// Also override print to go through our logger
		lua["print"] = [](sol::variadic_args va) {
			std::string result;
			for (auto v : va)
			{
				if (!result.empty())
					result += "\t";
				result += v.as<std::string>();
			}
			CB_CORE_INFO("[Lua] {0}", result);
		};
	}

	void LuaBindings::RegisterComponents(sol::state& lua)
	{
		// Component presence checks (added to Entity usertype via script-side metatable)
		// These are registered as free functions that take an Entity
		auto entity_type = lua["Entity"];
		if (!entity_type.valid())
			return;

		sol::usertype<Entity> et = entity_type;

		// Has-component checks
		et["HasRigidBody"] = [](Entity& e) -> bool { return e.HasComponent<RigidBodyComponent>(); };
		et["HasCollider"] = [](Entity& e) -> bool { return e.HasComponent<ColliderComponent>(); };
		et["HasMeshRenderer"] = [](Entity& e) -> bool { return e.HasComponent<MeshRendererComponent>(); };
		et["HasVoxelRenderer"] = [](Entity& e) -> bool { return e.HasComponent<VoxelRendererComponent>(); };
		et["HasDirectionalLight"] = [](Entity& e) -> bool { return e.HasComponent<DirectionalLightComponent>(); };
		et["HasScript"] = [](Entity& e) -> bool { return e.HasComponent<ScriptComponent>(); };
		et["HasCamera"] = [](Entity& e) -> bool { return e.HasComponent<CameraComponent>(); };

		// Camera accessors
		et["GetFOV"] = [](Entity& e) -> float {
			if (e.HasComponent<CameraComponent>())
				return e.GetComponent<CameraComponent>().FOV;
			return 0.0f;
		};
		et["SetFOV"] = [](Entity& e, float fov) {
			if (e.HasComponent<CameraComponent>())
				e.GetComponent<CameraComponent>().FOV = fov;
		};
		et["GetNearClip"] = [](Entity& e) -> float {
			if (e.HasComponent<CameraComponent>())
				return e.GetComponent<CameraComponent>().NearClip;
			return 0.0f;
		};
		et["SetNearClip"] = [](Entity& e, float nearClip) {
			if (e.HasComponent<CameraComponent>())
				e.GetComponent<CameraComponent>().NearClip = nearClip;
		};
		et["GetFarClip"] = [](Entity& e) -> float {
			if (e.HasComponent<CameraComponent>())
				return e.GetComponent<CameraComponent>().FarClip;
			return 0.0f;
		};
		et["SetFarClip"] = [](Entity& e, float farClip) {
			if (e.HasComponent<CameraComponent>())
				e.GetComponent<CameraComponent>().FarClip = farClip;
		};
		et["IsPrimaryCamera"] = [](Entity& e) -> bool {
			if (e.HasComponent<CameraComponent>())
				return e.GetComponent<CameraComponent>().Primary;
			return false;
		};
		et["SetPrimaryCamera"] = [](Entity& e, bool primary) {
			if (!e.HasComponent<CameraComponent>()) return;
			if (primary)
			{
				Scene* scene = e.GetScene();
				if (scene)
				{
					auto view = scene->GetRegistry().view<CameraComponent>();
					for (auto ent : view)
						view.get<CameraComponent>(ent).Primary = false;
				}
			}
			e.GetComponent<CameraComponent>().Primary = primary;
		};
	}

	void LuaBindings::RegisterPhysics(sol::state& lua)
	{
		// BodyType constants
		auto bodyType = lua.create_named_table("BodyType");
		bodyType["Static"] = "static";
		bodyType["Dynamic"] = "dynamic";
		bodyType["Kinematic"] = "kinematic";

		// --- RigidBodyProxy usertype ---
		// Internal name uses __ prefix; the global "RigidBody" is a component token table
		lua.new_usertype<RigidBodyProxy>("__RigidBodyProxy",
			// Forces & impulses
			"AddForce", [](RigidBodyProxy& rb, const Vector3& force) {
				if (!rb.IsValid()) { CB_CORE_WARN("[Lua] RigidBody:AddForce - body not ready"); return; }
				auto* world = rb.GetWorld();
				if (!world) return;
				world->GetBodyInterface().AddForce(rb.GetBodyID(), JPH::Vec3(force.x, force.y, force.z));
			},
			"AddTorque", [](RigidBodyProxy& rb, const Vector3& torque) {
				if (!rb.IsValid()) { CB_CORE_WARN("[Lua] RigidBody:AddTorque - body not ready"); return; }
				auto* world = rb.GetWorld();
				if (!world) return;
				world->GetBodyInterface().AddTorque(rb.GetBodyID(), JPH::Vec3(torque.x, torque.y, torque.z));
			},
			"AddImpulse", [](RigidBodyProxy& rb, const Vector3& impulse) {
				if (!rb.IsValid()) { CB_CORE_WARN("[Lua] RigidBody:AddImpulse - body not ready"); return; }
				auto* world = rb.GetWorld();
				if (!world) return;
				world->GetBodyInterface().AddImpulse(rb.GetBodyID(), JPH::Vec3(impulse.x, impulse.y, impulse.z));
			},

			// Velocity
			"GetLinearVelocity", [](RigidBodyProxy& rb) -> Vector3 {
				if (!rb.IsValid()) return Vector3(0.0f);
				auto* world = rb.GetWorld();
				if (!world) return Vector3(0.0f);
				JPH::Vec3 vel = world->GetBodyInterface().GetLinearVelocity(rb.GetBodyID());
				return Vector3(vel.GetX(), vel.GetY(), vel.GetZ());
			},
			"SetLinearVelocity", [](RigidBodyProxy& rb, const Vector3& vel) {
				if (!rb.IsValid()) return;
				auto* world = rb.GetWorld();
				if (!world) return;
				world->GetBodyInterface().SetLinearVelocity(rb.GetBodyID(), JPH::Vec3(vel.x, vel.y, vel.z));
			},
			"GetAngularVelocity", [](RigidBodyProxy& rb) -> Vector3 {
				if (!rb.IsValid()) return Vector3(0.0f);
				auto* world = rb.GetWorld();
				if (!world) return Vector3(0.0f);
				JPH::Vec3 vel = world->GetBodyInterface().GetAngularVelocity(rb.GetBodyID());
				return Vector3(vel.GetX(), vel.GetY(), vel.GetZ());
			},
			"SetAngularVelocity", [](RigidBodyProxy& rb, const Vector3& vel) {
				if (!rb.IsValid()) return;
				auto* world = rb.GetWorld();
				if (!world) return;
				world->GetBodyInterface().SetAngularVelocity(rb.GetBodyID(), JPH::Vec3(vel.x, vel.y, vel.z));
			},

			// Properties
			"GetMass", [](RigidBodyProxy& rb) -> float {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return 0.0f;
				return rb.OwnerEntity.GetComponent<RigidBodyComponent>().Mass;
			},
			"SetMass", [](RigidBodyProxy& rb, float mass) {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return;
				rb.OwnerEntity.GetComponent<RigidBodyComponent>().Mass = mass;
			},
			"GetFriction", [](RigidBodyProxy& rb) -> float {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return 0.0f;
				return rb.OwnerEntity.GetComponent<RigidBodyComponent>().Friction;
			},
			"SetFriction", [](RigidBodyProxy& rb, float friction) {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return;
				rb.OwnerEntity.GetComponent<RigidBodyComponent>().Friction = friction;
			},
			"GetRestitution", [](RigidBodyProxy& rb) -> float {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return 0.0f;
				return rb.OwnerEntity.GetComponent<RigidBodyComponent>().Restitution;
			},
			"SetRestitution", [](RigidBodyProxy& rb, float restitution) {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return;
				rb.OwnerEntity.GetComponent<RigidBodyComponent>().Restitution = restitution;
			},
			"GetLinearDamping", [](RigidBodyProxy& rb) -> float {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return 0.0f;
				return rb.OwnerEntity.GetComponent<RigidBodyComponent>().LinearDamping;
			},
			"SetLinearDamping", [](RigidBodyProxy& rb, float damping) {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return;
				rb.OwnerEntity.GetComponent<RigidBodyComponent>().LinearDamping = damping;
			},
			"GetAngularDamping", [](RigidBodyProxy& rb) -> float {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return 0.0f;
				return rb.OwnerEntity.GetComponent<RigidBodyComponent>().AngularDamping;
			},
			"SetAngularDamping", [](RigidBodyProxy& rb, float damping) {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return;
				rb.OwnerEntity.GetComponent<RigidBodyComponent>().AngularDamping = damping;
			},
			"IsUsingGravity", [](RigidBodyProxy& rb) -> bool {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return false;
				return rb.OwnerEntity.GetComponent<RigidBodyComponent>().UseGravity;
			},
			"SetUseGravity", [](RigidBodyProxy& rb, bool useGravity) {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return;
				rb.OwnerEntity.GetComponent<RigidBodyComponent>().UseGravity = useGravity;
			},
			"GetBodyType", [](RigidBodyProxy& rb) -> std::string {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return "none";
				switch (rb.OwnerEntity.GetComponent<RigidBodyComponent>().Type) {
				case BodyType::Static:    return "static";
				case BodyType::Dynamic:   return "dynamic";
				case BodyType::Kinematic: return "kinematic";
				}
				return "unknown";
			},
			"SetBodyType", [](RigidBodyProxy& rb, const std::string& type) {
				if (!rb.OwnerEntity || !rb.OwnerEntity.HasComponent<RigidBodyComponent>()) return;
				auto& comp = rb.OwnerEntity.GetComponent<RigidBodyComponent>();
				if (type == "static") comp.Type = BodyType::Static;
				else if (type == "dynamic") comp.Type = BodyType::Dynamic;
				else if (type == "kinematic") comp.Type = BodyType::Kinematic;
			},
			"IsValid", [](RigidBodyProxy& rb) -> bool { return rb.IsValid(); }
		);

		// --- TransformProxy usertype ---
		lua.new_usertype<TransformProxy>("__TransformProxy",
			"GetPosition", [](TransformProxy& tp) -> Vector3 {
				if (!tp.IsValid()) return Vector3(0.0f);
				return tp.OwnerEntity.GetComponent<TransformComponent>().Position;
			},
			"SetPosition", [](TransformProxy& tp, const Vector3& pos) {
				if (!tp.IsValid()) return;
				auto& tc = tp.OwnerEntity.GetComponent<TransformComponent>();
				tc.Position = pos;
				tc.Dirty = true;
			},
			"GetWorldPosition", [](TransformProxy& tp) -> Vector3 {
				if (!tp.IsValid()) return Vector3(0.0f);
				return tp.OwnerEntity.GetComponent<TransformComponent>().GetWorldPosition();
			},
			"GetRotation", [](TransformProxy& tp) -> Vector3 {
				if (!tp.IsValid()) return Vector3(0.0f);
				return tp.OwnerEntity.GetComponent<TransformComponent>().Rotation;
			},
			"SetRotation", [](TransformProxy& tp, const Vector3& rot) {
				if (!tp.IsValid()) return;
				auto& tc = tp.OwnerEntity.GetComponent<TransformComponent>();
				tc.Rotation = rot;
				tc.Dirty = true;
			},
			"GetScale", [](TransformProxy& tp) -> Vector3 {
				if (!tp.IsValid()) return Vector3(1.0f);
				return tp.OwnerEntity.GetComponent<TransformComponent>().Scale;
			},
			"SetScale", [](TransformProxy& tp, const Vector3& scale) {
				if (!tp.IsValid()) return;
				auto& tc = tp.OwnerEntity.GetComponent<TransformComponent>();
				tc.Scale = scale;
				tc.Dirty = true;
			},
			"GetForward", [](TransformProxy& tp) -> Vector3 {
				if (!tp.IsValid()) return Vector3(0.0f, 0.0f, 1.0f);
				return tp.OwnerEntity.GetComponent<TransformComponent>().GetForward();
			},
			"GetRight", [](TransformProxy& tp) -> Vector3 {
				if (!tp.IsValid()) return Vector3(1.0f, 0.0f, 0.0f);
				return tp.OwnerEntity.GetComponent<TransformComponent>().GetRight();
			},
			"GetUp", [](TransformProxy& tp) -> Vector3 {
				if (!tp.IsValid()) return Vector3(0.0f, 1.0f, 0.0f);
				return tp.OwnerEntity.GetComponent<TransformComponent>().GetUp();
			},
			"IsValid", [](TransformProxy& tp) -> bool { return tp.IsValid(); }
		);

		// --- CameraProxy usertype ---
		lua.new_usertype<CameraProxy>("__CameraProxy",
			"GetFOV", [](CameraProxy& cp) -> float {
				if (!cp.IsValid()) return 0.0f;
				return cp.OwnerEntity.GetComponent<CameraComponent>().FOV;
			},
			"SetFOV", [](CameraProxy& cp, float fov) {
				if (!cp.IsValid()) return;
				cp.OwnerEntity.GetComponent<CameraComponent>().FOV = fov;
			},
			"GetNearClip", [](CameraProxy& cp) -> float {
				if (!cp.IsValid()) return 0.0f;
				return cp.OwnerEntity.GetComponent<CameraComponent>().NearClip;
			},
			"SetNearClip", [](CameraProxy& cp, float nearClip) {
				if (!cp.IsValid()) return;
				cp.OwnerEntity.GetComponent<CameraComponent>().NearClip = nearClip;
			},
			"GetFarClip", [](CameraProxy& cp) -> float {
				if (!cp.IsValid()) return 0.0f;
				return cp.OwnerEntity.GetComponent<CameraComponent>().FarClip;
			},
			"SetFarClip", [](CameraProxy& cp, float farClip) {
				if (!cp.IsValid()) return;
				cp.OwnerEntity.GetComponent<CameraComponent>().FarClip = farClip;
			},
			"IsPrimary", [](CameraProxy& cp) -> bool {
				if (!cp.IsValid()) return false;
				return cp.OwnerEntity.GetComponent<CameraComponent>().Primary;
			},
			"SetPrimary", [](CameraProxy& cp, bool primary) {
				if (!cp.IsValid()) return;
				if (primary)
				{
					Scene* scene = cp.OwnerEntity.GetScene();
					if (scene)
					{
						auto view = scene->GetRegistry().view<CameraComponent>();
						for (auto e : view)
							view.get<CameraComponent>(e).Primary = false;
					}
				}
				cp.OwnerEntity.GetComponent<CameraComponent>().Primary = primary;
			},
			"IsValid", [](CameraProxy& cp) -> bool { return cp.IsValid(); }
		);

		// --- ColliderProxy usertype ---
		lua.new_usertype<ColliderProxy>("__ColliderProxy",
			"GetShape", [](ColliderProxy& cp) -> std::string {
				if (!cp.IsValid()) return "none";
				switch (cp.OwnerEntity.GetComponent<ColliderComponent>().Shape) {
				case ColliderShape::Box:           return "box";
				case ColliderShape::Sphere:        return "sphere";
				case ColliderShape::Capsule:       return "capsule";
				case ColliderShape::VoxelCompound: return "voxel_compound";
				}
				return "unknown";
			},
			"IsTrigger", [](ColliderProxy& cp) -> bool {
				if (!cp.IsValid()) return false;
				return cp.OwnerEntity.GetComponent<ColliderComponent>().IsTrigger;
			},
			"GetOffset", [](ColliderProxy& cp) -> Vector3 {
				if (!cp.IsValid()) return Vector3(0.0f);
				return cp.OwnerEntity.GetComponent<ColliderComponent>().Offset;
			},
			"SetOffset", [](ColliderProxy& cp, const Vector3& offset) {
				if (!cp.IsValid()) return;
				cp.OwnerEntity.GetComponent<ColliderComponent>().Offset = offset;
			},
			"GetHalfExtents", [](ColliderProxy& cp) -> Vector3 {
				if (!cp.IsValid()) return Vector3(0.5f);
				return cp.OwnerEntity.GetComponent<ColliderComponent>().HalfExtents;
			},
			"GetRadius", [](ColliderProxy& cp) -> float {
				if (!cp.IsValid()) return 0.0f;
				return cp.OwnerEntity.GetComponent<ColliderComponent>().Radius;
			},
			"IsValid", [](ColliderProxy& cp) -> bool { return cp.IsValid(); }
		);

		// --- MeshRendererProxy usertype ---
		lua.new_usertype<MeshRendererProxy>("__MeshRendererProxy",
			"IsVisible", [](MeshRendererProxy& mp) -> bool {
				if (!mp.IsValid()) return false;
				return mp.OwnerEntity.GetComponent<MeshRendererComponent>().Visible;
			},
			"SetVisible", [](MeshRendererProxy& mp, bool visible) {
				if (!mp.IsValid()) return;
				mp.OwnerEntity.GetComponent<MeshRendererComponent>().Visible = visible;
			},
			"IsValid", [](MeshRendererProxy& mp) -> bool { return mp.IsValid(); }
		);

		// --- VoxelRendererProxy usertype ---
		lua.new_usertype<VoxelRendererProxy>("__VoxelRendererProxy",
			"IsVisible", [](VoxelRendererProxy& vp) -> bool {
				if (!vp.IsValid()) return false;
				return vp.OwnerEntity.GetComponent<VoxelRendererComponent>().Visible;
			},
			"SetVisible", [](VoxelRendererProxy& vp, bool visible) {
				if (!vp.IsValid()) return;
				vp.OwnerEntity.GetComponent<VoxelRendererComponent>().Visible = visible;
			},
			"IsValid", [](VoxelRendererProxy& vp) -> bool { return vp.IsValid(); }
		);

		// --- DirectionalLightProxy usertype ---
		lua.new_usertype<DirectionalLightProxy>("__DirectionalLightProxy",
			"GetColor", [](DirectionalLightProxy& lp) -> Vector3 {
				if (!lp.IsValid()) return Vector3(1.0f);
				return lp.OwnerEntity.GetComponent<DirectionalLightComponent>().Color;
			},
			"SetColor", [](DirectionalLightProxy& lp, const Vector3& color) {
				if (!lp.IsValid()) return;
				lp.OwnerEntity.GetComponent<DirectionalLightComponent>().Color = color;
			},
			"GetIntensity", [](DirectionalLightProxy& lp) -> float {
				if (!lp.IsValid()) return 0.0f;
				return lp.OwnerEntity.GetComponent<DirectionalLightComponent>().Intensity;
			},
			"SetIntensity", [](DirectionalLightProxy& lp, float intensity) {
				if (!lp.IsValid()) return;
				lp.OwnerEntity.GetComponent<DirectionalLightComponent>().Intensity = intensity;
			},
			"IsCastingShadows", [](DirectionalLightProxy& lp) -> bool {
				if (!lp.IsValid()) return false;
				return lp.OwnerEntity.GetComponent<DirectionalLightComponent>().CastShadows;
			},
			"SetCastShadows", [](DirectionalLightProxy& lp, bool cast) {
				if (!lp.IsValid()) return;
				lp.OwnerEntity.GetComponent<DirectionalLightComponent>().CastShadows = cast;
			},
			"IsVisible", [](DirectionalLightProxy& lp) -> bool {
				if (!lp.IsValid()) return false;
				return lp.OwnerEntity.GetComponent<DirectionalLightComponent>().Visible;
			},
			"SetVisible", [](DirectionalLightProxy& lp, bool visible) {
				if (!lp.IsValid()) return;
				lp.OwnerEntity.GetComponent<DirectionalLightComponent>().Visible = visible;
			},
			"IsValid", [](DirectionalLightProxy& lp) -> bool { return lp.IsValid(); }
		);

		// --- Component token globals ---
		// Each is a plain table with __component = "TypeName" used by GetComponent dispatch
		auto makeToken = [&](const char* name) {
			sol::table t = lua.create_table();
			t["__component"] = name;
			lua[name] = t;
		};
		makeToken("RigidBody");
		makeToken("Transform");
		makeToken("Camera");
		makeToken("Collider");
		makeToken("MeshRenderer");
		makeToken("VoxelRenderer");
		makeToken("DirectionalLight");

		// Physics bindings on Entity
		auto entity_type = lua["Entity"];
		if (!entity_type.valid())
			return;

		sol::usertype<Entity> et = entity_type;

		// --- RigidBody properties (legacy direct accessors on Entity, kept for backwards compat) ---
		et["GetMass"] = [](Entity& e) -> float {
			if (e.HasComponent<RigidBodyComponent>())
				return e.GetComponent<RigidBodyComponent>().Mass;
			return 0.0f;
		};
		et["SetMass"] = [](Entity& e, float mass) {
			if (e.HasComponent<RigidBodyComponent>())
				e.GetComponent<RigidBodyComponent>().Mass = mass;
		};
		et["GetFriction"] = [](Entity& e) -> float {
			if (e.HasComponent<RigidBodyComponent>())
				return e.GetComponent<RigidBodyComponent>().Friction;
			return 0.0f;
		};
		et["SetFriction"] = [](Entity& e, float friction) {
			if (e.HasComponent<RigidBodyComponent>())
				e.GetComponent<RigidBodyComponent>().Friction = friction;
		};
		et["GetRestitution"] = [](Entity& e) -> float {
			if (e.HasComponent<RigidBodyComponent>())
				return e.GetComponent<RigidBodyComponent>().Restitution;
			return 0.0f;
		};
		et["SetRestitution"] = [](Entity& e, float restitution) {
			if (e.HasComponent<RigidBodyComponent>())
				e.GetComponent<RigidBodyComponent>().Restitution = restitution;
		};
		et["GetLinearDamping"] = [](Entity& e) -> float {
			if (e.HasComponent<RigidBodyComponent>())
				return e.GetComponent<RigidBodyComponent>().LinearDamping;
			return 0.0f;
		};
		et["SetLinearDamping"] = [](Entity& e, float damping) {
			if (e.HasComponent<RigidBodyComponent>())
				e.GetComponent<RigidBodyComponent>().LinearDamping = damping;
		};
		et["GetAngularDamping"] = [](Entity& e) -> float {
			if (e.HasComponent<RigidBodyComponent>())
				return e.GetComponent<RigidBodyComponent>().AngularDamping;
			return 0.0f;
		};
		et["SetAngularDamping"] = [](Entity& e, float damping) {
			if (e.HasComponent<RigidBodyComponent>())
				e.GetComponent<RigidBodyComponent>().AngularDamping = damping;
		};
		et["IsUsingGravity"] = [](Entity& e) -> bool {
			if (e.HasComponent<RigidBodyComponent>())
				return e.GetComponent<RigidBodyComponent>().UseGravity;
			return false;
		};
		et["SetUseGravity"] = [](Entity& e, bool useGravity) {
			if (e.HasComponent<RigidBodyComponent>())
				e.GetComponent<RigidBodyComponent>().UseGravity = useGravity;
		};
		et["GetBodyType"] = [](Entity& e) -> std::string {
			if (!e.HasComponent<RigidBodyComponent>())
				return "none";
			switch (e.GetComponent<RigidBodyComponent>().Type)
			{
			case BodyType::Static:    return "static";
			case BodyType::Dynamic:   return "dynamic";
			case BodyType::Kinematic: return "kinematic";
			}
			return "unknown";
		};
		et["SetBodyType"] = [](Entity& e, const std::string& type) {
			if (!e.HasComponent<RigidBodyComponent>())
				return;
			auto& rb = e.GetComponent<RigidBodyComponent>();
			if (type == "static") rb.Type = BodyType::Static;
			else if (type == "dynamic") rb.Type = BodyType::Dynamic;
			else if (type == "kinematic") rb.Type = BodyType::Kinematic;
			else CB_CORE_WARN("[Lua] Unknown body type: {0}", type);
		};

		// --- Forces & velocity (require active physics body) ---
		// These need to access the PhysicsWorld through the scene stored in self._scene
		// We use lambdas that take Entity and a scene pointer stored in the Lua self table

		// Physics force/velocity functions on Scene (use: self._scene:AddForce(self._entity, force))
		auto scene_type = lua["Scene"];
		if (!scene_type.valid())
			return;

		sol::usertype<Scene> st = scene_type;

		st["AddForce"] = [](Scene& scene, Entity entity, const Vector3& force) {
			if (!entity.HasComponent<RigidBodyComponent>()) {
				CB_CORE_WARN("[Lua] AddForce: entity has no RigidBody");
				return;
			}
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (!rb.BodyCreated) {
				CB_CORE_WARN("[Lua] AddForce: physics body not yet created");
				return;
			}
			auto* world = scene.GetPhysicsWorld();
			if (!world) return;
			world->GetBodyInterface().AddForce(rb.RuntimeBodyID,
				JPH::Vec3(force.x, force.y, force.z));
		};

		st["AddTorque"] = [](Scene& scene, Entity entity, const Vector3& torque) {
			if (!entity.HasComponent<RigidBodyComponent>()) {
				CB_CORE_WARN("[Lua] AddTorque: entity has no RigidBody");
				return;
			}
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (!rb.BodyCreated) {
				CB_CORE_WARN("[Lua] AddTorque: physics body not yet created");
				return;
			}
			auto* world = scene.GetPhysicsWorld();
			if (!world) return;
			world->GetBodyInterface().AddTorque(rb.RuntimeBodyID,
				JPH::Vec3(torque.x, torque.y, torque.z));
		};

		st["AddImpulse"] = [](Scene& scene, Entity entity, const Vector3& impulse) {
			if (!entity.HasComponent<RigidBodyComponent>()) {
				CB_CORE_WARN("[Lua] AddImpulse: entity has no RigidBody");
				return;
			}
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (!rb.BodyCreated) {
				CB_CORE_WARN("[Lua] AddImpulse: physics body not yet created");
				return;
			}
			auto* world = scene.GetPhysicsWorld();
			if (!world) return;
			world->GetBodyInterface().AddImpulse(rb.RuntimeBodyID,
				JPH::Vec3(impulse.x, impulse.y, impulse.z));
		};

		st["GetLinearVelocity"] = [](Scene& scene, Entity entity) -> Vector3 {
			if (!entity.HasComponent<RigidBodyComponent>())
				return Vector3(0.0f);
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (!rb.BodyCreated)
				return Vector3(0.0f);
			auto* world = scene.GetPhysicsWorld();
			if (!world) return Vector3(0.0f);
			JPH::Vec3 vel = world->GetBodyInterface().GetLinearVelocity(rb.RuntimeBodyID);
			return Vector3(vel.GetX(), vel.GetY(), vel.GetZ());
		};

		st["SetLinearVelocity"] = [](Scene& scene, Entity entity, const Vector3& vel) {
			if (!entity.HasComponent<RigidBodyComponent>()) return;
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (!rb.BodyCreated) return;
			auto* world = scene.GetPhysicsWorld();
			if (!world) return;
			world->GetBodyInterface().SetLinearVelocity(rb.RuntimeBodyID,
				JPH::Vec3(vel.x, vel.y, vel.z));
		};

		st["GetAngularVelocity"] = [](Scene& scene, Entity entity) -> Vector3 {
			if (!entity.HasComponent<RigidBodyComponent>())
				return Vector3(0.0f);
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (!rb.BodyCreated)
				return Vector3(0.0f);
			auto* world = scene.GetPhysicsWorld();
			if (!world) return Vector3(0.0f);
			JPH::Vec3 vel = world->GetBodyInterface().GetAngularVelocity(rb.RuntimeBodyID);
			return Vector3(vel.GetX(), vel.GetY(), vel.GetZ());
		};

		st["SetAngularVelocity"] = [](Scene& scene, Entity entity, const Vector3& vel) {
			if (!entity.HasComponent<RigidBodyComponent>()) return;
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (!rb.BodyCreated) return;
			auto* world = scene.GetPhysicsWorld();
			if (!world) return;
			world->GetBodyInterface().SetAngularVelocity(rb.RuntimeBodyID,
				JPH::Vec3(vel.x, vel.y, vel.z));
		};

		// Collider queries on Entity
		et["GetColliderShape"] = [](Entity& e) -> std::string {
			if (!e.HasComponent<ColliderComponent>())
				return "none";
			switch (e.GetComponent<ColliderComponent>().Shape)
			{
			case ColliderShape::Box:           return "box";
			case ColliderShape::Sphere:        return "sphere";
			case ColliderShape::Capsule:       return "capsule";
			case ColliderShape::VoxelCompound: return "voxel_compound";
			}
			return "unknown";
		};
		et["IsTrigger"] = [](Entity& e) -> bool {
			if (e.HasComponent<ColliderComponent>())
				return e.GetComponent<ColliderComponent>().IsTrigger;
			return false;
		};

		// --- Layer accessors on Entity ---
		et["GetLayer"] = [](Entity& e) -> int {
			if (e.HasComponent<IDComponent>())
				return static_cast<int>(e.GetComponent<IDComponent>().Layer);
			return 0;
		};
		et["SetLayer"] = [](Entity& e, int layer) {
			if (e.HasComponent<IDComponent>())
				e.GetComponent<IDComponent>().Layer = static_cast<uint8_t>(layer);
		};

		// --- Raycast on Scene ---
		// scene:Raycast(origin, dir, maxDist?, layerMask?, ignoreEntity?)
		st["Raycast"] = [](Scene& scene, const Vector3& origin, const Vector3& dir,
			sol::optional<float> maxDist, sol::optional<int> layerMask,
			sol::optional<Entity> ignoreEntity,
			sol::this_state L) -> sol::object {
			auto* world = scene.GetPhysicsWorld();
			if (!world) return sol::make_object(L, sol::nil);

			float dist = maxDist.value_or(1000.0f);
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();

			RaycastHit hit;
			if (world->Raycast(origin, dir, dist, hit, mask, ignoreUUID))
				return sol::make_object(L, hit);
			return sol::make_object(L, sol::nil);
		};

		// scene:RaycastAll(origin, dir, maxDist?, layerMask?, ignoreEntity?)
		st["RaycastAll"] = [](Scene& scene, const Vector3& origin, const Vector3& dir,
			sol::optional<float> maxDist, sol::optional<int> layerMask,
			sol::optional<Entity> ignoreEntity, sol::this_state L) -> sol::table {
			sol::state_view lua(L);
			sol::table result = lua.create_table();

			auto* world = scene.GetPhysicsWorld();
			if (!world) return result;

			float dist = maxDist.value_or(1000.0f);
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();

			auto hits = world->RaycastAll(origin, dir, dist, mask, ignoreUUID);
			for (size_t i = 0; i < hits.size(); i++)
				result[i + 1] = hits[i];
			return result;
		};

		// --- Layer constants ---
		auto layerTable = lua.create_named_table("Layer");
		layerTable["Default"] = 0;
		layerTable["Player"] = 1;
		layerTable["Enemy"] = 2;
		layerTable["Environment"] = 3;
		layerTable["Projectile"] = 4;
		layerTable["Trigger"] = 5;
		layerTable["IgnoreRaycast"] = 6;
		layerTable["All"] = 0xFFFF;

		layerTable["Mask"] = [](sol::variadic_args va) -> int {
			uint16_t mask = 0;
			for (auto v : va)
				mask |= static_cast<uint16_t>(1 << v.as<int>());
			return static_cast<int>(mask);
		};
	}

	void LuaBindings::RegisterDebug(sol::state& lua)
	{
		// RaycastHit usertype
		lua.new_usertype<RaycastHit>("RaycastHit",
			"point", &RaycastHit::Point,
			"normal", &RaycastHit::Normal,
			"fraction", &RaycastHit::Fraction,
			"distance", &RaycastHit::Distance,
			"GetEntity", [](RaycastHit& hit, Scene& scene) -> Entity {
				return scene.GetEntityByUUID(hit.EntityUUID);
			}
		);

		// Debug draw table
		auto debug = lua.create_named_table("Debug");

		debug["DrawLine"] = [](const Vector3& from, const Vector3& to,
			sol::optional<Vector3> color, sol::optional<float> duration) {
			DebugDraw::DrawLine(from, to,
				color.value_or(Vector3(0.0f, 1.0f, 0.0f)),
				duration.value_or(0.0f));
		};

		debug["DrawRay"] = [](const Vector3& origin, const Vector3& direction,
			sol::optional<float> maxDist, sol::optional<Vector3> color,
			sol::optional<float> duration) {
			DebugDraw::DrawRay(origin, direction,
				maxDist.value_or(100.0f),
				color.value_or(Vector3(1.0f, 0.0f, 0.0f)),
				duration.value_or(0.0f));
		};
	}

	void LuaBindings::RegisterFieldTypes(sol::state& lua)
	{
		lua["Float"] = [](sol::optional<float> def, sol::optional<float> min, sol::optional<float> max, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "float";
			t["default"] = def.value_or(0.0f);
			if (min) t["min"] = min.value();
			if (max) t["max"] = max.value();
			return t;
		};

		lua["Int"] = [](sol::optional<int> def, sol::optional<int> min, sol::optional<int> max, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "int";
			t["default"] = def.value_or(0);
			if (min) t["min"] = min.value();
			if (max) t["max"] = max.value();
			return t;
		};

		lua["Bool"] = [](sol::optional<bool> def, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "bool";
			t["default"] = def.value_or(false);
			return t;
		};

		lua["String"] = [](sol::optional<std::string> def, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "string";
			t["default"] = def.value_or("");
			return t;
		};

		lua["Color"] = [](sol::optional<float> r, sol::optional<float> g, sol::optional<float> b, sol::optional<float> a, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "color";
			sol::table def = sv.create_table();
			def[1] = r.value_or(1.0f);
			def[2] = g.value_or(1.0f);
			def[3] = b.value_or(1.0f);
			def[4] = a.value_or(1.0f);
			t["default"] = def;
			return t;
		};

		lua["Vector3"] = [](sol::optional<float> x, sol::optional<float> y, sol::optional<float> z, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "vector3";
			sol::table def = sv.create_table();
			def[1] = x.value_or(0.0f);
			def[2] = y.value_or(0.0f);
			def[3] = z.value_or(0.0f);
			t["default"] = def;
			return t;
		};
	}
}
