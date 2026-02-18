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
	};
}
