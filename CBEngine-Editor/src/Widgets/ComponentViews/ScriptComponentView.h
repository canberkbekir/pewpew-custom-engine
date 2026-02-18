#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "CBEngine/Utils/FileDialogs.h"

namespace CB
{
	class ScriptComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<ScriptComponent>())
				return;

			auto& script = entity.GetComponent<ScriptComponent>();

			if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// Script path
				char pathBuf[256];
				std::strncpy(pathBuf, script.ScriptPath.c_str(), sizeof(pathBuf) - 1);
				pathBuf[sizeof(pathBuf) - 1] = '\0';

				if (ImGui::InputText("Script Path", pathBuf, sizeof(pathBuf)))
				{
					script.ScriptPath = pathBuf;
				}

				ImGui::SameLine();
				if (ImGui::Button("...##ScriptBrowse"))
				{
					std::string path = FileDialogs::OpenFile("Lua Script (*.lua)\0*.lua\0");
					if (!path.empty())
					{
						script.ScriptPath = path;
						script.ScriptLoaded = false;
					}
				}

				// Status
				if (script.ScriptLoaded)
				{
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
				}
				else if (!script.ScriptPath.empty())
				{
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Not loaded (enter play mode)");
				}
				else
				{
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No script assigned");
				}

				// Reload button
				if (script.ScriptLoaded)
				{
					if (ImGui::Button("Reload"))
					{
						script.ScriptLoaded = false;
						ScriptEngine::ReloadScript(script.ScriptPath);
					}
				}
			}
		}
	};
}
