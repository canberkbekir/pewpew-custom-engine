#pragma once

namespace sol { class state; }

namespace CB
{
	class LuaBindings
	{
	public:
		static void RegisterAll(sol::state& lua);

	private:
		static void RegisterMath(sol::state& lua);
		static void RegisterInput(sol::state& lua);
		static void RegisterEntity(sol::state& lua);
		static void RegisterScene(sol::state& lua);
		static void RegisterLog(sol::state& lua);
		static void RegisterComponents(sol::state& lua);
		static void RegisterPhysics(sol::state& lua);
		static void RegisterDebug(sol::state& lua);
		static void RegisterFieldTypes(sol::state& lua);
		static void RegisterTime(sol::state& lua);
		static void RegisterLuaUtils(sol::state& lua);   // Phase 1: Signal, Coroutine, Tween, Pool
		static void RegisterAudio(sol::state& lua);      // Phase 5: Audio API
		static void RegisterSceneManagement(sol::state& lua); // Phase 6: Scene management
		static void RegisterHingeJoint(sol::state& lua); // HingeJoint constraint API
	};
}
