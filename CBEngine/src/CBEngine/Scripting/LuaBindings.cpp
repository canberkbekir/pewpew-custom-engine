#include "cbpch.h"
#include "LuaBindings.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Input/Input.h"
#include "CBEngine/Input/KeyCodes.h"
#include "CBEngine/Input/MouseButtonCodes.h"
#include "CBEngine/Math/CoreMath.h"

namespace CB
{
	void LuaBindings::RegisterAll(sol::state& lua)
	{
		RegisterMath(lua);
		RegisterInput(lua);
		RegisterEntity(lua);
		RegisterScene(lua);
		RegisterLog(lua);
	}

	void LuaBindings::RegisterMath(sol::state& lua)
	{
		// Vec3 type
		lua.new_usertype<Vector3>("Vec3",
			sol::constructors<Vector3(), Vector3(float), Vector3(float, float, float)>(),
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
			"GetUUID", [](Entity& e) -> uint64_t {
				return static_cast<uint64_t>(e.GetUUID());
			},
			"IsValid", [](Entity& e) -> bool { return static_cast<bool>(e); }
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
}
