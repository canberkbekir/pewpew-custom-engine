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
	};
}
