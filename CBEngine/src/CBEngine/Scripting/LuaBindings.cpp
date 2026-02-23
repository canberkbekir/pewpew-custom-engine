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
#include <glm/vec2.hpp>
#include <glm/common.hpp>
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Physics/PhysicsLayers.h"
#include "CBEngine/Debug/DebugDraw.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/BlueprintAsset.h"

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
		RegisterTime(lua);
	}

	void LuaBindings::RegisterMath(sol::state& lua)
	{
		// Vector2 type
		lua.new_usertype<glm::vec2>("Vector2",
			sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
			sol::call_constructor, sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
			"x", &glm::vec2::x,
			"y", &glm::vec2::y,
			sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
			sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
			sol::meta_function::multiplication, sol::overload(
				[](const glm::vec2& a, float s) { return a * s; },
				[](float s, const glm::vec2& a) { return s * a; }
			),
			"Length", [](const glm::vec2& v) { return glm::length(v); },
			"Normalized", [](const glm::vec2& v) { return glm::normalize(v); },
			"Dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); },
			"Lerp", [](const glm::vec2& a, const glm::vec2& b, float t) { return glm::mix(a, b, t); },
			"Distance", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance(a, b); }
		);

		// Vector3 type
		lua.new_usertype<Vector3>("Vector3",
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
			"Cross", [](const Vector3& a, const Vector3& b) { return glm::cross(a, b); },
			"Lerp", [](const Vector3& a, const Vector3& b, float t) { return glm::mix(a, b, t); },
			"Distance", [](const Vector3& a, const Vector3& b) { return glm::distance(a, b); }
		);

		// Vector3 static constants (set after usertype is registered)
		sol::table vec3Table = lua["Vector3"];
		vec3Table["zero"]    = Vector3(0.0f, 0.0f, 0.0f);
		vec3Table["one"]     = Vector3(1.0f, 1.0f, 1.0f);
		vec3Table["up"]      = Vector3(0.0f, 1.0f, 0.0f);
		vec3Table["down"]    = Vector3(0.0f, -1.0f, 0.0f);
		vec3Table["forward"] = Vector3(0.0f, 0.0f, 1.0f);
		vec3Table["back"]    = Vector3(0.0f, 0.0f, -1.0f);
		vec3Table["right"]   = Vector3(1.0f, 0.0f, 0.0f);
		vec3Table["left"]    = Vector3(-1.0f, 0.0f, 0.0f);

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

		// Keep lowercase math extensions — used internally by Timer.GetProgress()
		lua["math"]["clamp"] = [](float x, float mn, float mx) { return glm::clamp(x, mn, mx); };

		// Math — public capitalized API
		auto Math = lua.create_named_table("Math");
		// Constants
		Math["Pi"]          = 3.14159265358979323846f;
		// Rounding / integer
		Math["Floor"]       = [](float x) -> float { return std::floor(x); };
		Math["Ceil"]        = [](float x) -> float { return std::ceil(x); };
		Math["Round"]       = [](float x) -> float { return std::round(x); };
		Math["Abs"]         = [](float x) -> float { return std::abs(x); };
		Math["Sign"]        = [](float x) -> float { return (x > 0.0f) ? 1.0f : (x < 0.0f) ? -1.0f : 0.0f; };
		// Range / interpolation
		Math["Clamp"]       = [](float x, float mn, float mx) { return glm::clamp(x, mn, mx); };
		Math["Lerp"]        = [](float a, float b, float t) { return glm::mix(a, b, t); };
		Math["Min"]         = [](float a, float b) -> float { return a < b ? a : b; };
		Math["Max"]         = [](float a, float b) -> float { return a > b ? a : b; };
		Math["Smoothstep"]  = [](float edge0, float edge1, float x) { return glm::smoothstep(edge0, edge1, x); };
		Math["MoveTowards"] = [](float current, float target, float maxDelta) -> float {
			float diff = target - current;
			if (std::abs(diff) <= maxDelta) return target;
			return current + (diff > 0.0f ? maxDelta : -maxDelta);
		};
		Math["PingPong"]    = [](float t, float length) -> float {
			if (length <= 0.0f) return 0.0f;
			float mod = std::fmod(t, length * 2.0f);
			return length - std::abs(mod - length);
		};
		// Power / log
		Math["Sqrt"]        = [](float x) -> float { return std::sqrt(x); };
		Math["Pow"]         = [](float x, float y) -> float { return std::pow(x, y); };
		Math["Exp"]         = [](float x) -> float { return std::exp(x); };
		Math["Log"]         = [](float x) -> float { return std::log(x); };
		// Trig
		Math["Sin"]         = [](float x) -> float { return std::sin(x); };
		Math["Cos"]         = [](float x) -> float { return std::cos(x); };
		Math["Tan"]         = [](float x) -> float { return std::tan(x); };
		Math["Asin"]        = [](float x) -> float { return std::asin(x); };
		Math["Acos"]        = [](float x) -> float { return std::acos(x); };
		Math["Atan2"]       = [](float y, float x) -> float { return std::atan2(y, x); };
		Math["Rad"]         = [](float deg) -> float { return deg * (3.14159265358979323846f / 180.0f); };
		Math["Deg"]         = [](float rad) -> float { return rad * (180.0f / 3.14159265358979323846f); };

		// Timer utility — pure-Lua stopwatch/callback helper
		// Usage:  local t = Timer.New(0.5, function() ... end, true)
		//         t:Update(dt)   -- call in OnUpdate
		lua.script(R"(
Timer = {}
Timer.__index = Timer

function Timer.New(duration, callback, loop)
    local t = setmetatable({}, Timer)
    t._duration = duration or 1.0
    t._time     = 0.0
    t._callback = callback
    t._loop     = loop or false
    t._done     = false
    t._active   = true
    return t
end

function Timer:Update(dt)
    if not self._active or self._done then return end
    self._time = self._time + dt
    if self._time >= self._duration then
        if self._callback then self._callback() end
        if self._loop then
            self._time = self._time - self._duration
        else
            self._done   = true
            self._active = false
        end
    end
end

function Timer:Reset()
    self._time   = 0.0
    self._done   = false
    self._active = true
end

function Timer:Stop()  self._active = false end
function Timer:Start() self._active = true  end
function Timer:IsDone() return self._done end
function Timer:IsActive() return self._active end
function Timer:GetTime() return self._time end
function Timer:GetDuration() return self._duration end
function Timer:GetProgress()
    if self._duration <= 0 then return 1.0 end
    return math.clamp(self._time / self._duration, 0.0, 1.0)
end
)");
	}

	void LuaBindings::RegisterInput(sol::state& lua)
	{
		// Input table
		auto input = lua.create_named_table("Input");
		input["IsKeyPressed"]        = [](int keyCode) { return Input::IsKeyPressed(keyCode); };
		input["IsMouseButtonPressed"] = [](int button) { return Input::IsMouseButtonPressed(button); };
		input["GetMousePosition"] = []() -> std::pair<float, float> {
			auto [x, y] = Input::GetMousePosition();
			return {x, y};
		};
		input["GetMouseX"]        = []() { return Input::GetMouseX(); };
		input["GetMouseY"]        = []() { return Input::GetMouseY(); };
		input["IsKeyJustPressed"] = [](int keyCode) { return Input::IsKeyJustPressed(keyCode); };
		input["IsKeyJustReleased"] = [](int keyCode) { return Input::IsKeyJustReleased(keyCode); };

		// Returns a Vec2 with the raw mouse delta this frame
		input["GetMouseDelta"] = [](sol::this_state L) -> glm::vec2 {
			auto [dx, dy] = Input::GetMouseDelta();
			return glm::vec2(dx, dy);
		};

		// Returns a Vec2 with the scroll wheel delta this frame
		input["GetMouseScrollDelta"] = [](sol::this_state L) -> glm::vec2 {
			auto [sx, sy] = Input::GetMouseScrollDelta();
			return glm::vec2(sx, sy);
		};

		// Cursor lock — hides the cursor and enables raw mouse motion (FPS mode)
		input["SetCursorLocked"] = [](bool locked) { Input::SetCursorLocked(locked); };
		input["IsCursorLocked"]  = []() { return Input::IsCursorLocked(); };

		// Key constants — full GLFW key set
		auto key = lua.create_named_table("Key");
		// Printable keys
		key["Space"]     = CB_KEY_SPACE;
		key["Apostrophe"] = CB_KEY_APOSTROPHE;
		key["Comma"]     = CB_KEY_COMMA;
		key["Minus"]     = CB_KEY_MINUS;
		key["Period"]    = CB_KEY_PERIOD;
		key["Slash"]     = CB_KEY_SLASH;
		key["Semicolon"] = CB_KEY_SEMICOLON;
		key["Equal"]     = CB_KEY_EQUAL;
		key["LeftBracket"]  = CB_KEY_LEFT_BRACKET;
		key["Backslash"]    = CB_KEY_BACKSLASH;
		key["RightBracket"] = CB_KEY_RIGHT_BRACKET;
		key["GraveAccent"]  = CB_KEY_GRAVE_ACCENT;
		// Digits
		key["Num0"] = CB_KEY_0; key["Num1"] = CB_KEY_1; key["Num2"] = CB_KEY_2;
		key["Num3"] = CB_KEY_3; key["Num4"] = CB_KEY_4; key["Num5"] = CB_KEY_5;
		key["Num6"] = CB_KEY_6; key["Num7"] = CB_KEY_7; key["Num8"] = CB_KEY_8;
		key["Num9"] = CB_KEY_9;
		// Letters
		key["A"] = CB_KEY_A; key["B"] = CB_KEY_B; key["C"] = CB_KEY_C;
		key["D"] = CB_KEY_D; key["E"] = CB_KEY_E; key["F"] = CB_KEY_F;
		key["G"] = CB_KEY_G; key["H"] = CB_KEY_H; key["I"] = CB_KEY_I;
		key["J"] = CB_KEY_J; key["K"] = CB_KEY_K; key["L"] = CB_KEY_L;
		key["M"] = CB_KEY_M; key["N"] = CB_KEY_N; key["O"] = CB_KEY_O;
		key["P"] = CB_KEY_P; key["Q"] = CB_KEY_Q; key["R"] = CB_KEY_R;
		key["S"] = CB_KEY_S; key["T"] = CB_KEY_T; key["U"] = CB_KEY_U;
		key["V"] = CB_KEY_V; key["W"] = CB_KEY_W; key["X"] = CB_KEY_X;
		key["Y"] = CB_KEY_Y; key["Z"] = CB_KEY_Z;
		// Control keys
		key["Escape"]    = CB_KEY_ESCAPE;
		key["Enter"]     = CB_KEY_ENTER;
		key["Tab"]       = CB_KEY_TAB;
		key["Backspace"] = CB_KEY_BACKSPACE;
		key["Insert"]    = CB_KEY_INSERT;
		key["Delete"]    = CB_KEY_DELETE;
		key["PageUp"]    = CB_KEY_PAGE_UP;
		key["PageDown"]  = CB_KEY_PAGE_DOWN;
		key["Home"]      = CB_KEY_HOME;
		key["End"]       = CB_KEY_END;
		key["CapsLock"]  = CB_KEY_CAPS_LOCK;
		key["ScrollLock"] = CB_KEY_SCROLL_LOCK;
		key["NumLock"]   = CB_KEY_NUM_LOCK;
		key["PrintScreen"] = CB_KEY_PRINT_SCREEN;
		key["Pause"]     = CB_KEY_PAUSE;
		// Arrow keys
		key["Up"]    = CB_KEY_UP;
		key["Down"]  = CB_KEY_DOWN;
		key["Left"]  = CB_KEY_LEFT;
		key["Right"] = CB_KEY_RIGHT;
		// Function keys
		key["F1"]  = CB_KEY_F1;  key["F2"]  = CB_KEY_F2;  key["F3"]  = CB_KEY_F3;
		key["F4"]  = CB_KEY_F4;  key["F5"]  = CB_KEY_F5;  key["F6"]  = CB_KEY_F6;
		key["F7"]  = CB_KEY_F7;  key["F8"]  = CB_KEY_F8;  key["F9"]  = CB_KEY_F9;
		key["F10"] = CB_KEY_F10; key["F11"] = CB_KEY_F11; key["F12"] = CB_KEY_F12;
		// Modifiers
		key["LeftShift"]   = CB_KEY_LEFT_SHIFT;
		key["LeftControl"] = CB_KEY_LEFT_CONTROL;
		key["LeftAlt"]     = CB_KEY_LEFT_ALT;
		key["LeftSuper"]   = CB_KEY_LEFT_SUPER;
		key["RightShift"]   = CB_KEY_RIGHT_SHIFT;
		key["RightControl"] = CB_KEY_RIGHT_CONTROL;
		key["RightAlt"]     = CB_KEY_RIGHT_ALT;
		key["RightSuper"]   = CB_KEY_RIGHT_SUPER;
		// Numpad
		key["KP0"] = CB_KEY_KP_0; key["KP1"] = CB_KEY_KP_1; key["KP2"] = CB_KEY_KP_2;
		key["KP3"] = CB_KEY_KP_3; key["KP4"] = CB_KEY_KP_4; key["KP5"] = CB_KEY_KP_5;
		key["KP6"] = CB_KEY_KP_6; key["KP7"] = CB_KEY_KP_7; key["KP8"] = CB_KEY_KP_8;
		key["KP9"] = CB_KEY_KP_9;
		key["KPDecimal"]  = CB_KEY_KP_DECIMAL;
		key["KPDivide"]   = CB_KEY_KP_DIVIDE;
		key["KPMultiply"] = CB_KEY_KP_MULTIPLY;
		key["KPSubtract"] = CB_KEY_KP_SUBTRACT;
		key["KPAdd"]      = CB_KEY_KP_ADD;
		key["KPEnter"]    = CB_KEY_KP_ENTER;
		key["KPEqual"]    = CB_KEY_KP_EQUAL;
		// Mouse buttons — unified into the Key table for use with IsKeyPressed/IsKeyJustPressed
		key["MouseLeft"]    = CB_KEY_MOUSE_LEFT;
		key["MouseRight"]   = CB_KEY_MOUSE_RIGHT;
		key["MouseMiddle"]  = CB_KEY_MOUSE_MIDDLE;
		key["MouseX1"]      = CB_KEY_MOUSE_X1;   // side button 1
		key["MouseX2"]      = CB_KEY_MOUSE_X2;   // side button 2
		key["MouseButton6"] = CB_KEY_MOUSE_BUTTON6;
		key["MouseButton7"] = CB_KEY_MOUSE_BUTTON7;
		key["MouseButton8"] = CB_KEY_MOUSE_BUTTON8;

		// Mouse button constants (kept for backward compatibility)
		auto mouse = lua.create_named_table("Mouse");
		mouse["Left"]   = CB_MOUSE_BUTTON_LEFT;
		mouse["Right"]  = CB_MOUSE_BUTTON_RIGHT;
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
			"GetUUID", [](Entity& e) -> lua_Integer {
				return static_cast<lua_Integer>(static_cast<uint64_t>(e.GetUUID()));
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
			},

			// Local-space aliases (explicit naming; Rotation/Position/Scale are always local)
			"GetLocalPosition", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().Position;
				return Vector3(0.0f);
			},
			"SetLocalPosition", [](Entity& e, const Vector3& pos) {
				if (e.HasComponent<TransformComponent>())
				{
					e.GetComponent<TransformComponent>().Position = pos;
					e.GetComponent<TransformComponent>().Dirty = true;
				}
			},
			"GetLocalRotation", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().Rotation;
				return Vector3(0.0f);
			},
			"SetLocalRotation", [](Entity& e, const Vector3& rot) {
				if (e.HasComponent<TransformComponent>())
				{
					e.GetComponent<TransformComponent>().Rotation = rot;
					e.GetComponent<TransformComponent>().Dirty = true;
				}
			},
			"GetLocalScale", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
					return e.GetComponent<TransformComponent>().Scale;
				return Vector3(1.0f);
			},
			"SetLocalScale", [](Entity& e, const Vector3& scale) {
				if (e.HasComponent<TransformComponent>())
				{
					e.GetComponent<TransformComponent>().Scale = scale;
					e.GetComponent<TransformComponent>().Dirty = true;
				}
			},

			// Transform shorthand helpers
			"Translate", [](Entity& e, const Vector3& delta) {
				if (e.HasComponent<TransformComponent>())
				{
					auto& tc = e.GetComponent<TransformComponent>();
					tc.Position += delta;
					tc.Dirty = true;
				}
			},
			"Rotate", [](Entity& e, const Vector3& delta) {
				if (e.HasComponent<TransformComponent>())
				{
					auto& tc = e.GetComponent<TransformComponent>();
					tc.Rotation += delta;
					tc.Dirty = true;
				}
			},
			"GetWorldRotation", [](Entity& e) -> Vector3 {
				if (e.HasComponent<TransformComponent>())
				{
					auto& tc = e.GetComponent<TransformComponent>();
					glm::quat q = glm::quat_cast(glm::mat3(tc.WorldMatrix));
					return glm::degrees(glm::eulerAngles(q));
				}
				return Vector3(0.0f);
			},
			"LookAt", [](Entity& e, const Vector3& target, sol::optional<Vector3> upOpt) {
				if (!e.HasComponent<TransformComponent>()) return;
				auto& tc = e.GetComponent<TransformComponent>();
				Vector3 worldPos = tc.GetWorldPosition();
				Vector3 dir = target - worldPos;
				float len = glm::length(dir);
				if (len < 0.0001f) return;
				dir /= len;
				Vector3 up = upOpt.value_or(Vector3(0.0f, 1.0f, 0.0f));
				glm::quat q = glm::quatLookAt(dir, up);
				tc.Rotation = glm::degrees(glm::eulerAngles(q));
				tc.Dirty = true;
			}
		);
	}

	void LuaBindings::RegisterScene(sol::state& lua)
	{
		// BlueprintHandle usertype — wraps an asset UUID, never exposes raw paths to Lua
		lua.new_usertype<BlueprintHandle>("BlueprintHandle",
			"IsValid",  &BlueprintHandle::IsValid,
			"IsAsset",  &BlueprintHandle::IsAsset,
			"IsEntity", &BlueprintHandle::IsEntity
		);

		// Scene usertype
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
			"GetEntityByUUID", [](Scene& scene, lua_Integer uuid) -> Entity {
				return scene.GetEntityByUUID(UUID(static_cast<uint64_t>(uuid)));
			},
			"EntityExists", [](Scene& scene, lua_Integer uuid) -> bool {
				return scene.EntityExists(UUID(static_cast<uint64_t>(uuid)));
			},
			"InstantiateBlueprint", [](Scene& scene, BlueprintHandle handle, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table result = sv.create_table();

			if (!handle.IsValid())
			{
				CB_CORE_WARN("[Lua] Scene:InstantiateBlueprint - handle is not valid");
				return result;
			}

			String yaml;

			if (handle.IsAsset())
			{
				// .blueprint file — load via AssetManager (UUID survives renames)
				auto blueprint = AssetManager::GetAsset<BlueprintAsset>(handle.AssetUUID);
				if (!blueprint || blueprint->YAMLData.empty())
				{
					CB_CORE_WARN("[Lua] Scene:InstantiateBlueprint - asset not found (UUID={0})", static_cast<uint64_t>(handle.AssetUUID));
					return result;
				}
				yaml = blueprint->YAMLData;
			}
			else
			{
				// Scene entity — serialize its hierarchy on demand
				Entity templateEntity = scene.GetEntityByUUID(handle.EntityUUID);
				if (!templateEntity)
				{
					CB_CORE_WARN("[Lua] Scene:InstantiateBlueprint - template entity not found (UUID={0})", static_cast<uint64_t>(handle.EntityUUID));
					return result;
				}
				yaml = SceneSerializer::SerializeEntityHierarchy(templateEntity);
			}

			if (yaml.empty()) return result;

			Ref<Scene> sceneRef(&scene, [](Scene*) {});
			SceneSerializer serializer(sceneRef);
			std::vector<Entity> entities = serializer.InstantiateBlueprint(yaml);
			for (int i = 0; i < static_cast<int>(entities.size()); i++)
				result[i + 1] = entities[i];
			return result;
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

		// --- Physic table — standalone queries, all take scene as first arg ---
		// Usage: Physic.Raycast(self._scene, origin, dir, maxDist?, layerMask?, ignoreEntity?)
		auto physic = lua.create_named_table("Physic");

		physic["Raycast"] = [](Scene& scene, const Vector3& origin, const Vector3& dir,
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

		physic["RaycastAll"] = [](Scene& scene, const Vector3& origin, const Vector3& dir,
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

		physic["OverlapSphere"] = [](Scene& scene, const Vector3& center, float radius,
			sol::optional<int> layerMask, sol::optional<Entity> ignoreEntity,
			sol::this_state L) -> sol::table {
			sol::state_view lua(L);
			sol::table result = lua.create_table();
			auto* world = scene.GetPhysicsWorld();
			if (!world) return result;
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();
			auto hits = world->OverlapSphere(center, radius, mask, ignoreUUID);
			for (size_t i = 0; i < hits.size(); i++)
				result[i + 1] = hits[i];
			return result;
		};

		physic["OverlapBox"] = [](Scene& scene, const Vector3& center, const Vector3& halfExtents,
			sol::optional<int> layerMask, sol::optional<Entity> ignoreEntity,
			sol::this_state L) -> sol::table {
			sol::state_view lua(L);
			sol::table result = lua.create_table();
			auto* world = scene.GetPhysicsWorld();
			if (!world) return result;
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();
			auto hits = world->OverlapBox(center, halfExtents, mask, ignoreUUID);
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

	void LuaBindings::RegisterTime(sol::state& lua)
	{
		sol::table time = lua.create_named_table("Time");
		time["DeltaTime"] = 0.0f;
		time["TotalTime"] = 0.0f;
		time["FrameCount"] = static_cast<lua_Integer>(0);
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

		// Vec3 = Vector3 field descriptor for __fields blocks only.
		// The actual Vector3 math type is registered separately in RegisterMath.
		lua["Vec3"] = [](sol::optional<float> x, sol::optional<float> y, sol::optional<float> z, sol::this_state L) -> sol::table {
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

		// EntityRef() — editor drag target; injects Entity handle at runtime
		lua["EntityRef"] = [](sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "entityref";
			return t;
		};

		// ComponentRef(componentToken) — editor drag target; injects component proxy at runtime
		// Usage: ComponentRef(RigidBody)  where RigidBody is the global component token
		lua["ComponentRef"] = [](sol::table token, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "componentref";
			sol::object compName = token["__component"];
			if (compName.valid() && compName.get_type() == sol::type::string)
				t["subtype"] = compName.as<std::string>();
			return t;
		};

		// ScriptRef(ClassTable) — editor drag target; injects script instance at runtime
		// Usage: ScriptRef(PlayerMovement)  where PlayerMovement is the global class table
		lua["ScriptRef"] = [](sol::table classTable, sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "scriptref";
			// Find the global name that matches this class table pointer
			sol::table globals = sv.globals();
			for (auto& pair : globals)
			{
				if (pair.first.get_type() != sol::type::string) continue;
				if (pair.second.get_type() != sol::type::table) continue;
				sol::table tbl = pair.second;
				if (tbl.pointer() == classTable.pointer())
				{
					t["subtype"] = pair.first.as<std::string>();
					break;
				}
			}
			return t;
		};

		// BlueprintRef() -- editor drag target for .blueprint assets; injects path string at runtime
		lua["BlueprintRef"] = [](sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "blueprintref";
			return t;
		};
	}
}
