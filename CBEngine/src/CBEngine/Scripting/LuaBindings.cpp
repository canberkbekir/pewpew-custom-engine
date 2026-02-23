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
#include "CBEngine/Components/AudioSourceComponent.h"
#include "CBEngine/Input/Input.h"
#include "CBEngine/Input/KeyCodes.h"
#include "CBEngine/Input/MouseButtonCodes.h"
#include "CBEngine/Math/CoreMath.h"
#include <glm/vec2.hpp>
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Physics/PhysicsLayers.h"
#include "CBEngine/Debug/DebugDraw.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/BlueprintAsset.h"
#include "CBEngine/Audio/AudioEngine.h"
#include "CBEngine/Audio/AudioClipAsset.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "CBEngine/Core/Application.h"
#include "CBEngine/Renderer/Resources/Material.h"

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
		RegisterLuaUtils(lua);       // Phase 1: Signal, Coroutine, Tween, Pool
		RegisterAudio(lua);          // Phase 5: Audio API
		RegisterSceneManagement(lua);// Phase 6: Scene management
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

		// Quat type
		lua.new_usertype<glm::quat>("Quat",
			sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
			sol::call_constructor, sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
			"w", &glm::quat::w,
			"x", &glm::quat::x,
			"y", &glm::quat::y,
			"z", &glm::quat::z,
			// Instance methods
			"EulerAngles", [](const glm::quat& q) { return glm::degrees(glm::eulerAngles(q)); },
			"ToEuler",     [](const glm::quat& q) { return glm::degrees(glm::eulerAngles(q)); },
			"Rotate",      [](const glm::quat& q, const Vector3& v) { return q * v; },
			sol::meta_function::multiplication, sol::overload(
				[](const glm::quat& a, const glm::quat& b) { return a * b; },
				[](const glm::quat& q, const Vector3& v) { return q * v; }
			)
		);

		// Quat static helpers (set after usertype)
		sol::table quatTable = lua["Quat"];
		quatTable["FromEuler"] = [](const Vector3& euler) -> glm::quat {
			return glm::quat(glm::radians(euler));
		};
		quatTable["LookRotation"] = [](const Vector3& forward, sol::optional<Vector3> upOpt) -> glm::quat {
			Vector3 fwd = glm::normalize(forward);
			Vector3 up  = glm::normalize(upOpt.value_or(Vector3(0.0f, 1.0f, 0.0f)));
			// glm::quatLookAt points -Z at forward; we flip to match engine convention
			return glm::quatLookAt(fwd, up);
		};
		quatTable["Slerp"] = [](const glm::quat& a, const glm::quat& b, float t) -> glm::quat {
			return glm::slerp(a, b, t);
		};
		quatTable["AngleBetween"] = [](const glm::quat& a, const glm::quat& b) -> float {
			return glm::degrees(glm::angle(glm::inverse(a) * b));
		};
		quatTable["Identity"] = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

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
			},

			// Clone — instantiate this entity hierarchy into the given scene
			"Clone", [](Entity& e, Scene& scene, sol::this_state L) -> sol::object {
				if (!e) return sol::make_object(L, sol::nil);
				String yaml = SceneSerializer::SerializeEntityHierarchy(e);
				if (yaml.empty()) return sol::make_object(L, sol::nil);
				Ref<Scene> sceneRef(&scene, [](Scene*) {});
				SceneSerializer serializer(sceneRef);
				auto entities = serializer.InstantiateBlueprint(yaml);
				if (entities.empty()) return sol::make_object(L, sol::nil);
				return sol::make_object(L, entities[0]);
			}
		);
	}

	void LuaBindings::RegisterScene(sol::state& lua)
	{
		// BlueprintHandle usertype — wraps an asset UUID, never exposes raw paths to Lua
		lua.new_usertype<BlueprintHandle>("BlueprintHandle",
			"IsValid", &BlueprintHandle::IsValid
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
			// Find ALL entities with a given name (returns Lua table)
			"FindAllWithName", [](Scene& scene, const std::string& name, sol::this_state L) -> sol::table {
				sol::state_view sv(L);
				sol::table result = sv.create_table();
				auto view = scene.GetRegistry().view<TagComponent>();
				int idx = 1;
				for (auto entity : view)
				{
					auto& tag = view.get<TagComponent>(entity);
					if (tag.Tag == name)
						result[idx++] = Entity{entity, &scene};
				}
				return result;
			},
			// Get ALL entities in the scene
			"GetAllEntities", [](Scene& scene, sol::this_state L) -> sol::table {
				sol::state_view sv(L);
				sol::table result = sv.create_table();
				auto view = scene.GetRegistry().view<IDComponent>();
				int idx = 1;
				for (auto entity : view)
					result[idx++] = Entity{entity, &scene};
				return result;
			},
			// Get entity count
			"GetEntityCount", [](Scene& scene) -> int {
				return static_cast<int>(scene.GetRegistry().view<IDComponent>().size());
			},
			// Find entities within radius — uses OverlapSphere
			"FindEntitiesInRadius", [](Scene& scene, const Vector3& center, float radius,
				sol::optional<int> layerMask, sol::this_state L) -> sol::table {
				sol::state_view sv(L);
				sol::table result = sv.create_table();
				auto* world = scene.GetPhysicsWorld();
				if (!world) return result;
				uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
				auto hits = world->OverlapSphere(center, radius, mask, UUID());
				int idx = 1;
				for (auto& hit : hits)
				{
					Entity e = scene.GetEntityByUUID(hit.EntityUUID);
					if (e) result[idx++] = e;
				}
				return result;
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
			"Instantiate", sol::overload(
				// Overload 1: BlueprintHandle (.blueprint asset by UUID)
				[](Scene& scene, BlueprintHandle handle, sol::this_state L) -> sol::table {
					sol::state_view sv(L);
					sol::table result = sv.create_table();
					if (!handle.IsValid())
					{
						CB_CORE_WARN("[Lua] Scene:Instantiate - BlueprintHandle is not valid");
						return result;
					}
					auto blueprint = AssetManager::GetAsset<BlueprintAsset>(handle.AssetUUID);
					if (!blueprint || blueprint->YAMLData.empty())
					{
						CB_CORE_WARN("[Lua] Scene:Instantiate - blueprint asset not found (UUID={0})", static_cast<uint64_t>(handle.AssetUUID));
						return result;
					}
					Ref<Scene> sceneRef(&scene, [](Scene*) {});
					SceneSerializer serializer(sceneRef);
					std::vector<Entity> entities = serializer.InstantiateBlueprint(blueprint->YAMLData);
					for (int i = 0; i < static_cast<int>(entities.size()); i++)
						result[i + 1] = entities[i];
					return result;
				},
				// Overload 2: Entity (clone scene entity hierarchy)
				[](Scene& scene, Entity templateEntity, sol::this_state L) -> sol::table {
					sol::state_view sv(L);
					sol::table result = sv.create_table();
					if (!templateEntity)
					{
						CB_CORE_WARN("[Lua] Scene:Instantiate - template entity is not valid");
						return result;
					}
					String yaml = SceneSerializer::SerializeEntityHierarchy(templateEntity);
					if (yaml.empty()) return result;
					Ref<Scene> sceneRef(&scene, [](Scene*) {});
					SceneSerializer serializer(sceneRef);
					std::vector<Entity> entities = serializer.InstantiateBlueprint(yaml);
					for (int i = 0; i < static_cast<int>(entities.size()); i++)
						result[i + 1] = entities[i];
					return result;
				})
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
		et["HasAudioSource"] = [](Entity& e) -> bool { return e.HasComponent<AudioSourceComponent>(); };

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
			"IsValid", [](CameraProxy& cp) -> bool { return cp.IsValid(); },

			// --- Screen-space utilities (Phase 4) ---
			// Returns viewport pixel coords (origin top-left) or nil if behind camera
			"WorldToScreen", [](CameraProxy& cp, const Vector3& worldPos, sol::this_state L) -> sol::object {
				if (!cp.IsValid()) return sol::make_object(L, sol::nil);
				auto& cam = cp.OwnerEntity.GetComponent<CameraComponent>();

				// Build view-projection from camera component cached matrices
				if (!cp.OwnerEntity.HasComponent<TransformComponent>())
					return sol::make_object(L, sol::nil);
				auto& tc = cp.OwnerEntity.GetComponent<TransformComponent>();

				float w = static_cast<float>(Application::Get().GetWindow().GetWidth());
				float h = static_cast<float>(Application::Get().GetWindow().GetHeight());
				float aspect = (h > 0.0f) ? w / h : 16.0f / 9.0f;
				glm::mat4 proj = glm::perspective(glm::radians(cam.FOV), aspect, cam.NearClip, cam.FarClip);
				glm::mat4 view = glm::inverse(tc.WorldMatrix);
				glm::mat4 vp   = proj * view;

				glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
				if (clip.w <= 0.0f)
					return sol::make_object(L, sol::nil);

				glm::vec3 ndc = glm::vec3(clip) / clip.w;  // range -1..+1
				float px = (ndc.x *  0.5f + 0.5f) * w;
				float py = (ndc.y * -0.5f + 0.5f) * h;  // y flipped (top-left origin)
				return sol::make_object(L, glm::vec2(px, py));
			},

			// Returns {origin=Vector3, direction=Vector3} world-space ray from screen coords
			"ScreenToWorldRay", [](CameraProxy& cp, float sx, float sy, sol::this_state L) -> sol::object {
				if (!cp.IsValid()) return sol::make_object(L, sol::nil);
				auto& cam = cp.OwnerEntity.GetComponent<CameraComponent>();
				if (!cp.OwnerEntity.HasComponent<TransformComponent>())
					return sol::make_object(L, sol::nil);
				auto& tc = cp.OwnerEntity.GetComponent<TransformComponent>();

				float w = static_cast<float>(Application::Get().GetWindow().GetWidth());
				float h = static_cast<float>(Application::Get().GetWindow().GetHeight());
				float aspect = (h > 0.0f) ? w / h : 16.0f / 9.0f;

				glm::mat4 proj = glm::perspective(glm::radians(cam.FOV), aspect, cam.NearClip, cam.FarClip);
				glm::mat4 view = glm::inverse(tc.WorldMatrix);
				glm::mat4 invVP = glm::inverse(proj * view);

				// NDC from pixel
				float ndcX = (sx / w) * 2.0f - 1.0f;
				float ndcY = 1.0f - (sy / h) * 2.0f;

				glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
				glm::vec4 farPt  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
				nearPt /= nearPt.w;
				farPt  /= farPt.w;

				Vector3 origin(nearPt.x, nearPt.y, nearPt.z);
				Vector3 dir = glm::normalize(Vector3(farPt.x - nearPt.x, farPt.y - nearPt.y, farPt.z - nearPt.z));

				sol::state_view sv(L);
				sol::table result = sv.create_table();
				result["origin"]    = origin;
				result["direction"] = dir;
				return sol::make_object(L, result);
			}
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

		// --- MaterialProxy usertype ---
		lua.new_usertype<MaterialProxy>("__MaterialProxy",
			"GetAlbedo", [](MaterialProxy& mp) -> Vector3 {
				if (!mp.IsValid()) return Vector3(1.0f);
				return mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->GetAlbedo();
			},
			"SetAlbedo", [](MaterialProxy& mp, const Vector3& color) {
				if (!mp.IsValid()) return;
				mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->SetAlbedo(color);
			},
			"GetMetallic", [](MaterialProxy& mp) -> float {
				if (!mp.IsValid()) return 0.0f;
				return mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->GetMetallic();
			},
			"SetMetallic", [](MaterialProxy& mp, float v) {
				if (!mp.IsValid()) return;
				mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->SetMetallic(v);
			},
			"GetRoughness", [](MaterialProxy& mp) -> float {
				if (!mp.IsValid()) return 0.5f;
				return mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->GetRoughness();
			},
			"SetRoughness", [](MaterialProxy& mp, float v) {
				if (!mp.IsValid()) return;
				mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->SetRoughness(v);
			},
			"GetSmoothShading", [](MaterialProxy& mp) -> float {
				if (!mp.IsValid()) return 1.0f;
				return mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->GetSmoothShading();
			},
			"SetSmoothShading", [](MaterialProxy& mp, float v) {
				if (!mp.IsValid()) return;
				mp.OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset->SetSmoothShading(v);
			},
			"IsValid", [](MaterialProxy& mp) -> bool { return mp.IsValid(); }
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
			"GetMaterial", [](MeshRendererProxy& mp, sol::this_state L) -> sol::object {
				if (!mp.IsValid()) return sol::make_object(L, sol::nil);
				auto& comp = mp.OwnerEntity.GetComponent<MeshRendererComponent>();
				if (!comp.MaterialAsset) return sol::make_object(L, sol::nil);
				return sol::make_object(L, MaterialProxy{mp.OwnerEntity});
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

		// --- AudioSourceProxy usertype ---
		lua.new_usertype<AudioSourceProxy>("__AudioSourceProxy",
			"IsValid", [](AudioSourceProxy& asp) -> bool { return asp.IsValid(); },
			"Play", [](AudioSourceProxy& asp) {
				if (!asp.IsValid()) return;
				auto& comp = asp.OwnerEntity.GetComponent<AudioSourceComponent>();
				if (comp.ClipHandle.IsValid())
				{
					AudioHandle handle(comp.ClipHandle);
					Vector3 pos = asp.OwnerEntity.HasComponent<TransformComponent>()
						? asp.OwnerEntity.GetComponent<TransformComponent>().GetWorldPosition()
						: Vector3(0.0f);
					comp.RuntimeSourceID = comp.Is3D
						? AudioEngine::PlayAt(handle, pos, comp.Volume)
						: AudioEngine::Play(handle, comp.Volume);
				}
			},
			"Stop", [](AudioSourceProxy& asp) {
				if (!asp.IsValid()) return;
				AudioEngine::Stop(asp.OwnerEntity.GetComponent<AudioSourceComponent>().RuntimeSourceID);
			},
			"Pause", [](AudioSourceProxy& asp) {
				if (!asp.IsValid()) return;
				AudioEngine::Pause(asp.OwnerEntity.GetComponent<AudioSourceComponent>().RuntimeSourceID);
			},
			"Resume", [](AudioSourceProxy& asp) {
				if (!asp.IsValid()) return;
				AudioEngine::Resume(asp.OwnerEntity.GetComponent<AudioSourceComponent>().RuntimeSourceID);
			},
			"SetVolume", [](AudioSourceProxy& asp, float v) {
				if (!asp.IsValid()) return;
				auto& comp = asp.OwnerEntity.GetComponent<AudioSourceComponent>();
				comp.Volume = v;
				AudioEngine::SetVolume(comp.RuntimeSourceID, v);
			},
			"SetPitch", [](AudioSourceProxy& asp, float p) {
				if (!asp.IsValid()) return;
				asp.OwnerEntity.GetComponent<AudioSourceComponent>().Pitch = p;
			},
			"IsPlaying", [](AudioSourceProxy& asp) -> bool {
				if (!asp.IsValid()) return false;
				return AudioEngine::IsPlaying(asp.OwnerEntity.GetComponent<AudioSourceComponent>().RuntimeSourceID);
			}
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
		makeToken("AudioSource");

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

		// --- Sweep casts ---
		physic["SphereCast"] = [](Scene& scene, const Vector3& origin, const Vector3& dir, float radius,
			sol::optional<float> maxDist, sol::optional<int> layerMask,
			sol::optional<Entity> ignoreEntity, sol::this_state L) -> sol::object {
			sol::state_view lua(L);
			auto* world = scene.GetPhysicsWorld();
			if (!world) return sol::make_object(lua, sol::nil);
			float dist = maxDist.value_or(100.0f);
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();
			RaycastHit outHit;
			if (world->SphereCast(origin, dir, radius, dist, outHit, mask, ignoreUUID))
				return sol::make_object(lua, outHit);
			return sol::make_object(lua, sol::nil);
		};

		physic["BoxCast"] = [](Scene& scene, const Vector3& origin, const Vector3& dir, const Vector3& halfExtents,
			sol::optional<float> maxDist, sol::optional<int> layerMask,
			sol::optional<Entity> ignoreEntity, sol::this_state L) -> sol::object {
			sol::state_view lua(L);
			auto* world = scene.GetPhysicsWorld();
			if (!world) return sol::make_object(lua, sol::nil);
			float dist = maxDist.value_or(100.0f);
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();
			RaycastHit outHit;
			if (world->BoxCast(origin, dir, halfExtents, dist, outHit, mask, ignoreUUID))
				return sol::make_object(lua, outHit);
			return sol::make_object(lua, sol::nil);
		};

		physic["SphereCastAll"] = [](Scene& scene, const Vector3& origin, const Vector3& dir, float radius,
			sol::optional<float> maxDist, sol::optional<int> layerMask,
			sol::optional<Entity> ignoreEntity, sol::this_state L) -> sol::table {
			sol::state_view lua(L);
			sol::table result = lua.create_table();
			auto* world = scene.GetPhysicsWorld();
			if (!world) return result;
			float dist = maxDist.value_or(100.0f);
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();
			auto hits = world->SphereCastAll(origin, dir, radius, dist, mask, ignoreUUID);
			for (size_t i = 0; i < hits.size(); i++)
				result[i + 1] = hits[i];
			return result;
		};

		physic["BoxCastAll"] = [](Scene& scene, const Vector3& origin, const Vector3& dir, const Vector3& halfExtents,
			sol::optional<float> maxDist, sol::optional<int> layerMask,
			sol::optional<Entity> ignoreEntity, sol::this_state L) -> sol::table {
			sol::state_view lua(L);
			sol::table result = lua.create_table();
			auto* world = scene.GetPhysicsWorld();
			if (!world) return result;
			float dist = maxDist.value_or(100.0f);
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();
			auto hits = world->BoxCastAll(origin, dir, halfExtents, dist, mask, ignoreUUID);
			for (size_t i = 0; i < hits.size(); i++)
				result[i + 1] = hits[i];
			return result;
		};

		physic["OverlapCapsule"] = [](Scene& scene, const Vector3& center, float radius, float halfHeight,
			sol::optional<int> layerMask, sol::optional<Entity> ignoreEntity,
			sol::this_state L) -> sol::table {
			sol::state_view lua(L);
			sol::table result = lua.create_table();
			auto* world = scene.GetPhysicsWorld();
			if (!world) return result;
			uint16_t mask = static_cast<uint16_t>(layerMask.value_or(0xFFFF));
			UUID ignoreUUID = (ignoreEntity.has_value() && ignoreEntity.value())
				? ignoreEntity.value().GetUUID() : UUID();
			auto hits = world->OverlapCapsule(center, radius, halfHeight, mask, ignoreUUID);
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

		// DrawSphere — 3 great-circle arcs (XY, XZ, YZ planes)
		debug["DrawSphere"] = [](const Vector3& center, float radius,
			sol::optional<Vector3> color, sol::optional<float> duration) {
			Vector3 col = color.value_or(Vector3(0.0f, 1.0f, 0.0f));
			float dur = duration.value_or(0.0f);
			const int segs = 24;
			float step = 2.0f * 3.14159265f / segs;
			for (int i = 0; i < segs; i++)
			{
				float a0 = i * step, a1 = (i + 1) * step;
				float c0 = std::cos(a0) * radius, s0 = std::sin(a0) * radius;
				float c1 = std::cos(a1) * radius, s1 = std::sin(a1) * radius;
				// XY plane
				DebugDraw::DrawLine(center + Vector3(c0,s0,0), center + Vector3(c1,s1,0), col, dur);
				// XZ plane
				DebugDraw::DrawLine(center + Vector3(c0,0,s0), center + Vector3(c1,0,s1), col, dur);
				// YZ plane
				DebugDraw::DrawLine(center + Vector3(0,c0,s0), center + Vector3(0,c1,s1), col, dur);
			}
		};

		// DrawBox — 12 edges of an AABB centered at center with given halfExtents
		debug["DrawBox"] = [](const Vector3& center, const Vector3& half,
			sol::optional<Vector3> color, sol::optional<float> duration) {
			Vector3 col = color.value_or(Vector3(1.0f, 1.0f, 0.0f));
			float dur = duration.value_or(0.0f);
			Vector3 v[8];
			v[0] = center + Vector3(-half.x,-half.y,-half.z);
			v[1] = center + Vector3( half.x,-half.y,-half.z);
			v[2] = center + Vector3( half.x, half.y,-half.z);
			v[3] = center + Vector3(-half.x, half.y,-half.z);
			v[4] = center + Vector3(-half.x,-half.y, half.z);
			v[5] = center + Vector3( half.x,-half.y, half.z);
			v[6] = center + Vector3( half.x, half.y, half.z);
			v[7] = center + Vector3(-half.x, half.y, half.z);
			// Bottom face
			DebugDraw::DrawLine(v[0],v[1],col,dur); DebugDraw::DrawLine(v[1],v[2],col,dur);
			DebugDraw::DrawLine(v[2],v[3],col,dur); DebugDraw::DrawLine(v[3],v[0],col,dur);
			// Top face
			DebugDraw::DrawLine(v[4],v[5],col,dur); DebugDraw::DrawLine(v[5],v[6],col,dur);
			DebugDraw::DrawLine(v[6],v[7],col,dur); DebugDraw::DrawLine(v[7],v[4],col,dur);
			// Verticals
			DebugDraw::DrawLine(v[0],v[4],col,dur); DebugDraw::DrawLine(v[1],v[5],col,dur);
			DebugDraw::DrawLine(v[2],v[6],col,dur); DebugDraw::DrawLine(v[3],v[7],col,dur);
		};

		// DrawWireCube — AABB from min to max corners
		debug["DrawWireCube"] = [](const Vector3& minP, const Vector3& maxP,
			sol::optional<Vector3> color, sol::optional<float> duration) {
			Vector3 col = color.value_or(Vector3(1.0f, 1.0f, 0.0f));
			float dur = duration.value_or(0.0f);
			Vector3 v[8];
			v[0] = Vector3(minP.x,minP.y,minP.z); v[1] = Vector3(maxP.x,minP.y,minP.z);
			v[2] = Vector3(maxP.x,maxP.y,minP.z); v[3] = Vector3(minP.x,maxP.y,minP.z);
			v[4] = Vector3(minP.x,minP.y,maxP.z); v[5] = Vector3(maxP.x,minP.y,maxP.z);
			v[6] = Vector3(maxP.x,maxP.y,maxP.z); v[7] = Vector3(minP.x,maxP.y,maxP.z);
			DebugDraw::DrawLine(v[0],v[1],col,dur); DebugDraw::DrawLine(v[1],v[2],col,dur);
			DebugDraw::DrawLine(v[2],v[3],col,dur); DebugDraw::DrawLine(v[3],v[0],col,dur);
			DebugDraw::DrawLine(v[4],v[5],col,dur); DebugDraw::DrawLine(v[5],v[6],col,dur);
			DebugDraw::DrawLine(v[6],v[7],col,dur); DebugDraw::DrawLine(v[7],v[4],col,dur);
			DebugDraw::DrawLine(v[0],v[4],col,dur); DebugDraw::DrawLine(v[1],v[5],col,dur);
			DebugDraw::DrawLine(v[2],v[6],col,dur); DebugDraw::DrawLine(v[3],v[7],col,dur);
		};

		// DrawCross — 3-axis cross at position
		debug["DrawCross"] = [](const Vector3& pos, float size,
			sol::optional<Vector3> color, sol::optional<float> duration) {
			Vector3 col = color.value_or(Vector3(1.0f, 0.0f, 1.0f));
			float dur = duration.value_or(0.0f);
			float h = size * 0.5f;
			DebugDraw::DrawLine(pos - Vector3(h,0,0), pos + Vector3(h,0,0), col, dur);
			DebugDraw::DrawLine(pos - Vector3(0,h,0), pos + Vector3(0,h,0), col, dur);
			DebugDraw::DrawLine(pos - Vector3(0,0,h), pos + Vector3(0,0,h), col, dur);
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

		// AudioRef() — editor drag target for .sfx assets; injects AudioHandle at runtime
		lua["AudioRef"] = [](sol::this_state L) -> sol::table {
			sol::state_view sv(L);
			sol::table t = sv.create_table();
			t["type"] = "audioclipref";
			return t;
		};
	}

	// =========================================================================
	// RegisterLuaUtils — Phase 1: Signal, Coroutine scheduler, Tween, Pool
	// =========================================================================
	void LuaBindings::RegisterLuaUtils(sol::state& lua)
	{
		lua.script(R"lua(
-- =====================================================================
-- Signal — lightweight typed event dispatcher (no string keys)
-- =====================================================================
Signal = {}
Signal.__index = Signal

function Signal.New()
    local s = setmetatable({}, Signal)
    s._listeners = {}
    return s
end

function Signal:Connect(fn)
    local conn = { _signal = self, _fn = fn, Connected = true }
    function conn:Disconnect()
        if not self.Connected then return end
        self.Connected = false
        local listeners = self._signal._listeners
        for i = #listeners, 1, -1 do
            if listeners[i] == self then
                table.remove(listeners, i)
                return
            end
        end
    end
    table.insert(self._listeners, conn)
    return conn
end

function Signal:Fire(...)
    -- Iterate over a copy so Disconnect() inside a listener is safe
    local list = {}
    for _, c in ipairs(self._listeners) do list[#list+1] = c end
    for _, c in ipairs(list) do
        if c.Connected then c._fn(...) end
    end
end

function Signal:DisconnectAll()
    for _, c in ipairs(self._listeners) do c.Connected = false end
    self._listeners = {}
end

-- =====================================================================
-- Coroutine — per-frame scheduler wrapping native Lua coroutines
-- =====================================================================
Coroutine = {}
Coroutine._active = {}

-- Wait(seconds) — yield inside a started coroutine
function Coroutine.Wait(seconds)
    coroutine.yield({ _type="wait", _t=seconds })
end

-- WaitFrames(n) — yield for N frames
function Coroutine.WaitFrames(n)
    coroutine.yield({ _type="frames", _n=(n or 1) })
end

-- Start(fn) — create, immediately resume, and register for future ticks
function Coroutine.Start(fn)
    local co = coroutine.create(fn)
    local ok, yielded = coroutine.resume(co)
    if not ok then
        -- Use rawget to avoid calling the overridden print
        Log.Error("[Coroutine] " .. tostring(yielded))
        return
    end
    if coroutine.status(co) ~= "dead" then
        table.insert(Coroutine._active, { co=co, wait=yielded })
    end
end

-- _Tick(dt) — called by ScriptEngine::BeginFrame each frame
function Coroutine._Tick(dt)
    local stillActive = {}
    for _, entry in ipairs(Coroutine._active) do
        local co = entry.co
        local w  = entry.wait
        local ready = false

        if w == nil then
            ready = true
        elseif type(w) == "table" then
            if w._type == "wait" then
                w._t = w._t - dt
                if w._t <= 0 then ready = true end
            elseif w._type == "frames" then
                w._n = w._n - 1
                if w._n <= 0 then ready = true end
            else
                ready = true
            end
        else
            ready = true
        end

        if ready then
            local ok, yielded = coroutine.resume(co)
            if not ok then
                Log.Error("[Coroutine] " .. tostring(yielded))
            elseif coroutine.status(co) ~= "dead" then
                table.insert(stillActive, { co=co, wait=yielded })
            end
        else
            table.insert(stillActive, entry)
        end
    end
    Coroutine._active = stillActive
end

-- =====================================================================
-- Easing functions
-- =====================================================================
Easing = {}

-- ---- Classic aliases (backward compat) ----
Easing.Linear    = function(t) return t end
Easing.EaseIn    = function(t) return t * t end
Easing.EaseOut   = function(t) return t * (2 - t) end
Easing.EaseInOut = function(t)
    if t < 0.5 then return 2*t*t else return -1 + (4 - 2*t)*t end
end

-- ---- Quad ----
Easing.QuadIn    = function(t) return t * t end
Easing.QuadOut   = function(t) return t * (2 - t) end
Easing.QuadInOut = function(t)
    if t < 0.5 then return 2*t*t else return -1 + (4 - 2*t)*t end
end

-- ---- Cubic ----
Easing.CubicIn    = function(t) return t * t * t end
Easing.CubicOut   = function(t) local u = t - 1; return u*u*u + 1 end
Easing.CubicInOut = function(t)
    if t < 0.5 then return 4*t*t*t
    else local u = 2*t - 2; return 0.5*u*u*u + 1 end
end

-- ---- Sine ----
Easing.SineIn    = function(t) return 1 - math.cos(t * math.pi * 0.5) end
Easing.SineOut   = function(t) return math.sin(t * math.pi * 0.5) end
Easing.SineInOut = function(t) return -(math.cos(math.pi * t) - 1) * 0.5 end

-- ---- Expo ----
Easing.ExpoIn    = function(t) return t == 0 and 0 or 2^(10*(t-1)) end
Easing.ExpoOut   = function(t) return t == 1 and 1 or 1 - 2^(-10*t) end
Easing.ExpoInOut = function(t)
    if t == 0 then return 0 end; if t == 1 then return 1 end
    if t < 0.5 then return 2^(20*t - 10) * 0.5
    else return (2 - 2^(-20*t + 10)) * 0.5 end
end

-- ---- Circ ----
Easing.CircIn    = function(t) return 1 - math.sqrt(1 - t*t) end
Easing.CircOut   = function(t) local u = t - 1; return math.sqrt(1 - u*u) end
Easing.CircInOut = function(t)
    if t < 0.5 then return (1 - math.sqrt(1 - (2*t)^2)) * 0.5
    else return (math.sqrt(1 - (-2*t + 2)^2) + 1) * 0.5 end
end

-- ---- Back (slight overshoot) ----
Easing.BackIn = function(t)
    local c = 1.70158
    return (c + 1)*t*t*t - c*t*t
end
Easing.BackOut = function(t)
    local c = 1.70158; local u = t - 1
    return (c + 1)*u*u*u + c*u*u + 1
end
Easing.BackInOut = function(t)
    local c = 1.70158 * 1.525
    if t < 0.5 then return ((2*t)^2 * ((c+1)*2*t - c)) * 0.5
    else local u = 2*t - 2; return (u^2 * ((c+1)*u + c) + 2) * 0.5 end
end

-- ---- Bounce ----
local function _bounceOut(t)
    if t < 1/2.75 then return 7.5625*t*t
    elseif t < 2/2.75 then t = t - 1.5/2.75; return 7.5625*t*t + 0.75
    elseif t < 2.5/2.75 then t = t - 2.25/2.75; return 7.5625*t*t + 0.9375
    else t = t - 2.625/2.75; return 7.5625*t*t + 0.984375 end
end
Easing.BounceOut   = _bounceOut
Easing.BounceIn    = function(t) return 1 - _bounceOut(1 - t) end
Easing.BounceInOut = function(t)
    if t < 0.5 then return (1 - _bounceOut(1 - 2*t)) * 0.5
    else return (1 + _bounceOut(2*t - 1)) * 0.5 end
end
Easing.Bounce = Easing.BounceOut   -- backward compat

-- ---- Elastic ----
Easing.ElasticOut = function(t)
    if t == 0 or t == 1 then return t end
    return 2^(-10*t) * math.sin((t*10 - 0.75) * (2*math.pi/3)) + 1
end
Easing.ElasticIn = function(t)
    if t == 0 or t == 1 then return t end
    return -(2^(10*t - 10)) * math.sin((t*10 - 10.75) * (2*math.pi/3))
end
Easing.ElasticInOut = function(t)
    if t == 0 or t == 1 then return t end
    local c = 2*math.pi/4.5
    if t < 0.5 then return -(2^(20*t - 10) * math.sin((20*t - 11.125) * c)) * 0.5
    else return (2^(-20*t + 10) * math.sin((20*t - 11.125) * c)) * 0.5 + 1 end
end
Easing.Elastic = Easing.ElasticOut  -- backward compat

-- =====================================================================
-- Tween — DOTween-style fluent tweening engine
-- =====================================================================
Tween = {}
Tween._active    = {}   -- active standalone tweens
Tween._sequences = {}   -- active sequences

-- Lerp helper: numbers, Vector3, or Vector2
local function _twLerp(a, b, t)
    local ta = type(a)
    if ta == "number" then
        return a + (b - a) * t
    elseif ta == "userdata" then
        return a:Lerp(b, t)
    end
    return b
end

-- Internal tween factory (not yet added to active list)
local function _newTween(from, to, duration)
    local tw = {
        _from      = from,
        _to        = to,
        _origFrom  = from,
        _origTo    = to,
        _duration  = duration or 1.0,
        _elapsed   = 0.0,
        _delay     = 0.0,
        _easing    = Easing.Linear,
        _onUpdate  = nil,
        _onComplete= nil,
        _onStart   = nil,
        _active    = true,
        _paused    = false,
        _started   = false,
        _loops     = 0,
        _loopType  = "Restart",
        _loopsDone = 0,
        _relative  = false,
    }
    -- Fluent setters
    function tw:SetEase(fn)    self._easing    = fn or Easing.Linear; return self end
    function tw:OnUpdate(fn)   self._onUpdate  = fn; return self end
    function tw:OnComplete(fn) self._onComplete = fn; return self end
    function tw:OnStart(fn)    self._onStart   = fn; return self end
    function tw:SetDelay(s)    self._delay     = s or 0; return self end
    function tw:SetLoops(n, lt)
        self._loops    = (n == nil) and -1 or n
        self._loopType = lt or "Restart"
        return self
    end
    function tw:SetRelative() self._relative = true; return self end
    -- Control
    function tw:Cancel()   self._active = false end
    function tw:Pause()    self._paused = true  end
    function tw:Resume()   self._paused = false end
    function tw:IsActive() return self._active  end
    return tw
end

-- Resolve relative target on first tick
local function _resolveRelative(tw)
    if not tw._relative then return end
    tw._relative = false
    local ta = type(tw._from)
    if ta == "number" then
        tw._to = tw._from + tw._to
    elseif ta == "userdata" then
        tw._to = tw._from + tw._to
    end
    tw._origTo = tw._to
end

-- Tick one tween; returns true if still alive
local function _tickTween(tw, dt)
    if not tw._active then return false end
    if tw._paused then return true end
    -- Delay phase
    if tw._delay > 0 then
        tw._delay = tw._delay - dt
        if tw._delay > 0 then return true end
        dt = -tw._delay; tw._delay = 0
    end
    -- First tick: resolve relative targets and fire OnStart
    if not tw._started then
        tw._started = true
        _resolveRelative(tw)
        if tw._onStart then tw._onStart() end
    end
    tw._elapsed = tw._elapsed + dt
    local t = tw._elapsed / tw._duration
    if t >= 1.0 then
        -- Fire at exactly 1.0 (end value)
        if tw._onUpdate then tw._onUpdate(_twLerp(tw._from, tw._to, tw._easing(1.0))) end
        local infinite = (tw._loops == -1)
        local hasMore  = infinite or (tw._loopsDone < tw._loops)
        if hasMore then
            tw._loopsDone = tw._loopsDone + 1
            tw._elapsed   = tw._elapsed - tw._duration
            if tw._loopType == "Yoyo" then
                tw._from, tw._to = tw._to, tw._from
            else
                tw._from = tw._origFrom
                tw._to   = tw._origTo
            end
            return true
        else
            if tw._onComplete then tw._onComplete() end
            tw._active = false
            return false
        end
    else
        if tw._onUpdate then tw._onUpdate(_twLerp(tw._from, tw._to, tw._easing(t))) end
        return true
    end
end

-- Tween.Value — main entry point
-- Fluent:  Tween.Value(from, to, duration):SetEase(Easing.EaseOut):OnUpdate(fn)
-- Legacy:  Tween.Value(from, to, duration, easing, onUpdate, onComplete)
function Tween.Value(from, to, duration, easingFn, onUpdate, onComplete)
    local tw = _newTween(from, to, duration)
    if easingFn   then tw._easing    = easingFn   end
    if onUpdate   then tw._onUpdate  = onUpdate   end
    if onComplete then tw._onComplete = onComplete end
    table.insert(Tween._active, tw)
    return tw
end

-- Kill helpers
function Tween.Kill(tween)
    if tween then tween._active = false end
end
function Tween.KillAll()
    for _, tw in ipairs(Tween._active)    do tw._active = false end
    for _, sq in ipairs(Tween._sequences) do sq._active = false end
end

-- Global tick — called by ScriptEngine::BeginFrame each frame
function Tween._Tick(dt)
    local alive = {}
    for _, tw in ipairs(Tween._active) do
        if _tickTween(tw, dt) then table.insert(alive, tw) end
    end
    Tween._active = alive
    local aliveSeq = {}
    for _, sq in ipairs(Tween._sequences) do
        if sq:_tick(dt) then table.insert(aliveSeq, sq) end
    end
    Tween._sequences = aliveSeq
end
)lua");

		lua.script(R"lua(
-- =====================================================================
-- Tween.Sequence — ordered chain / simultaneous group of tweens
-- =====================================================================
function Tween.Sequence()
    local seq = {
        _steps      = {},
        _duration   = 0.0,
        _elapsed    = 0.0,
        _delay      = 0.0,
        _active     = true,
        _paused     = false,
        _started    = false,
        _loops      = 0,
        _loopType   = "Restart",
        _loopsDone  = 0,
        _onComplete = nil,
        _onStart    = nil,
        _headTime   = 0.0,
    }

    -- Append: run tween after all previous tweens finish
    function seq:Append(tw)
        if not tw then return self end
        for i = #Tween._active, 1, -1 do
            if Tween._active[i] == tw then table.remove(Tween._active, i); break end
        end
        tw._elapsed = 0; tw._started = false; tw._active = true
        local step = { tween = tw, startTime = self._duration, completed = false }
        table.insert(self._steps, step)
        self._headTime = self._duration
        self._duration = self._duration + (tw._delay or 0) + tw._duration
        return self
    end

    -- Join: run tween simultaneously with the last Append
    function seq:Join(tw)
        if not tw then return self end
        for i = #Tween._active, 1, -1 do
            if Tween._active[i] == tw then table.remove(Tween._active, i); break end
        end
        tw._elapsed = 0; tw._started = false; tw._active = true
        local step = { tween = tw, startTime = self._headTime, completed = false }
        table.insert(self._steps, step)
        local stepEnd = self._headTime + (tw._delay or 0) + tw._duration
        if stepEnd > self._duration then self._duration = stepEnd end
        return self
    end

    -- AppendInterval: insert an empty gap
    function seq:AppendInterval(secs)
        self._headTime = self._duration
        self._duration = self._duration + secs
        return self
    end

    function seq:SetDelay(s)   self._delay = s or 0; return self end
    function seq:SetLoops(n, lt)
        self._loops    = (n == nil) and -1 or n
        self._loopType = lt or "Restart"
        return self
    end
    function seq:OnComplete(fn) self._onComplete = fn; return self end
    function seq:OnStart(fn)    self._onStart    = fn; return self end
    function seq:Cancel()   self._active = false end
    function seq:Pause()    self._paused = true  end
    function seq:Resume()   self._paused = false end
    function seq:IsActive() return self._active  end

    function seq:_tick(dt)
        if not self._active then return false end
        if self._paused then return true end
        if self._delay > 0 then
            self._delay = self._delay - dt
            if self._delay > 0 then return true end
            dt = -self._delay; self._delay = 0
        end
        if not self._started then
            self._started = true
            if self._onStart then self._onStart() end
        end
        self._elapsed = self._elapsed + dt
        -- Drive each step's tween using sequence-local time
        for _, step in ipairs(self._steps) do
            local tw = step.tween
            if tw and not step.completed then
                local twDelay = tw._delay or 0
                local twStart = step.startTime + twDelay
                if self._elapsed >= twStart then
                    if not tw._started then
                        tw._started = true
                        _resolveRelative(tw)
                        if tw._onStart then tw._onStart() end
                    end
                    local localT = math.min((self._elapsed - twStart) / tw._duration, 1.0)
                    local val = _twLerp(tw._from, tw._to, tw._easing(localT))
                    if tw._onUpdate then tw._onUpdate(val) end
                    if localT >= 1.0 then
                        step.completed = true
                        if tw._onComplete then tw._onComplete() end
                    end
                end
            end
        end
        -- Sequence completion
        if self._elapsed >= self._duration then
            local infinite = (self._loops == -1)
            local hasMore  = infinite or (self._loopsDone < self._loops)
            if hasMore then
                self._loopsDone = self._loopsDone + 1
                self._elapsed   = self._elapsed - self._duration
                if self._loopType == "Yoyo" then
                    for _, step in ipairs(self._steps) do
                        local tw = step.tween
                        tw._from, tw._to = tw._to, tw._from
                        tw._origFrom, tw._origTo = tw._origTo, tw._origFrom
                        step.startTime = self._duration - (step.startTime + tw._duration)
                        step.completed = false; tw._started = false
                    end
                else
                    for _, step in ipairs(self._steps) do
                        local tw = step.tween
                        tw._elapsed = 0; tw._started = false
                        tw._from = tw._origFrom; tw._to = tw._origTo
                        step.completed = false
                    end
                end
                return true
            else
                if self._onComplete then self._onComplete() end
                self._active = false
                return false
            end
        end
        return true
    end

    table.insert(Tween._sequences, seq)
    return seq
end
)lua");

		lua.script(R"lua(
-- =====================================================================
-- Pool — entity object pool
-- =====================================================================
Pool = {}
Pool.__index = Pool

-- Pool.New(scene, blueprintHandle, initSize?)
function Pool.New(scene, blueprintHandle, initSize)
    local p = setmetatable({}, Pool)
    p._scene     = scene
    p._blueprint = blueprintHandle
    p._available = {}
    if initSize and initSize > 0 then
        p:Prewarm(initSize)
    end
    return p
end

function Pool:Prewarm(count)
    for _ = 1, count do
        local entities = self._scene:Instantiate(self._blueprint)
        if entities and entities[1] then
            local e = entities[1]
            e:SetVisible(false)
            table.insert(self._available, e)
        end
    end
end

function Pool:Get()
    if #self._available > 0 then
        local e = table.remove(self._available)
        e:SetVisible(true)
        return e
    end
    -- Spawn new
    local entities = self._scene:Instantiate(self._blueprint)
    if entities and entities[1] then
        return entities[1]
    end
    return nil
end

function Pool:Return(entity)
    if not entity or not entity:IsValid() then return end
    entity:SetVisible(false)
    table.insert(self._available, entity)
end
)lua");

	}

	// =========================================================================
	// RegisterAudio — Phase 5: Audio.Play/Stop/etc + AudioHandle usertype
	// =========================================================================
	void LuaBindings::RegisterAudio(sol::state& lua)
	{
		// AudioHandle usertype — wraps asset UUID, never exposes raw paths
		lua.new_usertype<AudioHandle>("AudioHandle",
			"IsValid", &AudioHandle::IsValid
		);

		auto audio = lua.create_named_table("Audio");

		audio["Play"] = [](AudioHandle handle, sol::optional<float> volumeOverride) -> uint32_t {
			return AudioEngine::Play(handle, volumeOverride.value_or(-1.0f));
		};

		audio["PlayAt"] = [](AudioHandle handle, const Vector3& pos, sol::optional<float> volumeOverride) -> uint32_t {
			return AudioEngine::PlayAt(handle, pos, volumeOverride.value_or(-1.0f));
		};

		audio["Stop"] = [](uint32_t src) {
			AudioEngine::Stop(src);
		};

		audio["StopAll"] = []() {
			AudioEngine::StopAll();
		};

		audio["Pause"] = [](uint32_t src) {
			AudioEngine::Pause(src);
		};

		audio["Resume"] = [](uint32_t src) {
			AudioEngine::Resume(src);
		};

		audio["SetVolume"] = [](uint32_t src, float vol) {
			AudioEngine::SetVolume(src, vol);
		};

		audio["SetMasterVolume"] = [](float vol) {
			AudioEngine::SetMasterVolume(vol);
		};

		audio["IsPlaying"] = [](uint32_t src) -> bool {
			return AudioEngine::IsPlaying(src);
		};
	}

	// =========================================================================
	// RegisterSceneManagement — Phase 6: deferred scene load/reload from Lua
	// =========================================================================
	void LuaBindings::RegisterSceneManagement(sol::state& lua)
	{
		// Extend the existing Scene usertype (if present) with static scene methods
		sol::object sceneObj = lua["Scene"];
		if (sceneObj.valid() && sceneObj.get_type() == sol::type::userdata)
		{
			// We can't add static methods to an already-registered usertype at runtime;
			// instead, expose them as a plain table "SceneManager"
		}

		// SceneManager table — static scene management (not per-instance)
		// Also aliased onto the Scene usertype table for ergonomics:
		//   Scene.LoadScene("path")    → deferred load
		//   Scene.ReloadScene()        → restart current scene
		//   Scene.GetSceneName()       → string
		//   Scene.GetScenePath()       → string

		sol::table sceneMgr = lua.create_named_table("SceneManager");

		sceneMgr["LoadScene"] = [](const std::string& path) {
			ScriptEngine::RequestSceneLoad(path);
		};

		sceneMgr["ReloadScene"] = []() {
			ScriptEngine::RequestSceneReload();
		};

		sceneMgr["GetSceneName"] = []() -> std::string {
			return SceneManager::GetActiveSceneName();
		};

		sceneMgr["GetScenePath"] = []() -> std::string {
			return SceneManager::GetActiveScenePath();
		};

		// Also inject on the global "Scene" table so scripts can call Scene.LoadScene(...)
		sol::object existingScene = lua["Scene"];
		if (existingScene.get_type() == sol::type::userdata)
		{
			// Usertype — add as methods on usertype table
			sol::usertype<Scene> st = existingScene;
			// Instance-style wrappers (scene:LoadScene(...) is weird but kept for compat)
			st["LoadScene"]    = [](Scene& /*s*/, const std::string& path) {
				ScriptEngine::RequestSceneLoad(path);
			};
			st["ReloadScene"]  = [](Scene& /*s*/) {
				ScriptEngine::RequestSceneReload();
			};
			st["GetSceneName"] = [](Scene& /*s*/) -> std::string {
				return SceneManager::GetActiveSceneName();
			};
			st["GetScenePath"] = [](Scene& /*s*/) -> std::string {
				return SceneManager::GetActiveScenePath();
			};
		}
	}
}
