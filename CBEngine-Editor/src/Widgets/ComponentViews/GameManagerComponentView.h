#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/GameManagerComponent.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "../ComponentCard.h"

#include <filesystem>
#include <algorithm>

namespace CB
{
	class GameManagerComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<GameManagerComponent>())
				return;

			bool removed = false;
			bool reset = false;

			bool opened = ComponentCard::Begin("Game Manager", true, &removed, &reset);

			if (removed) {
				entity.RemoveComponent<GameManagerComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) {
				// Reset to base GameManager script
				if (entity.HasComponent<ScriptComponent>()) {
					auto& scriptComp = entity.GetComponent<ScriptComponent>();
					if (!scriptComp.Scripts.empty()) {
						scriptComp.Scripts[0].ScriptPath = "lua/GameManager.lua";
						scriptComp.Scripts[0].FieldsParsed = false;
						scriptComp.Scripts[0].ClassName = "";
						scriptComp.Scripts[0].Fields.clear();
					}
				}
			}

			if (opened) {
				// Ensure entity has a ScriptComponent with at least one entry
				if (!entity.HasComponent<ScriptComponent>()) {
					auto& sc = entity.AddComponent<ScriptComponent>();
					ScriptEntry gmScript;
					gmScript.ScriptPath = "lua/GameManager.lua";
					sc.Scripts.push_back(gmScript);
				}

				auto& scriptComp = entity.GetComponent<ScriptComponent>();
				if (scriptComp.Scripts.empty()) {
					ScriptEntry gmScript;
					gmScript.ScriptPath = "lua/GameManager.lua";
					scriptComp.Scripts.push_back(gmScript);
				}

				auto& gmEntry = scriptComp.Scripts[0];

				// Build list of available GM scripts
				std::vector<std::string> gmScripts = ScriptEngine::GetGameManagerScripts();
				std::sort(gmScripts.begin(), gmScripts.end());

				// Current selection display name
				std::string currentDisplay = "GameManager (Base)";
				int currentIndex = 0; // 0 = base

				if (gmEntry.ScriptPath != "lua/GameManager.lua" && !gmEntry.ScriptPath.empty()) {
					std::filesystem::path p(gmEntry.ScriptPath);
					currentDisplay = p.stem().string();

					// Find in list
					for (size_t i = 0; i < gmScripts.size(); i++) {
						if (gmScripts[i] == gmEntry.ScriptPath) {
							currentIndex = static_cast<int>(i) + 1; // +1 because 0 is base
							break;
						}
					}
				}

				// Dropdown
				ImGui::Text("GameManager Script");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##GMScript", currentDisplay.c_str())) {
					// Base option
					bool isBase = (gmEntry.ScriptPath == "lua/GameManager.lua");
					if (ImGui::Selectable("GameManager (Base)", isBase)) {
						gmEntry.ScriptPath = "lua/GameManager.lua";
						gmEntry.FieldsParsed = false;
						gmEntry.ClassName = "";
						gmEntry.Fields.clear();
					}
					if (isBase)
						ImGui::SetItemDefaultFocus();

					// Discovered GM-derived scripts
					for (size_t i = 0; i < gmScripts.size(); i++) {
						// Skip the base script itself
						if (gmScripts[i] == "lua/GameManager.lua")
							continue;

						std::filesystem::path p(gmScripts[i]);
						std::string displayName = p.stem().string();
						bool isSelected = (gmEntry.ScriptPath == gmScripts[i]);

						if (ImGui::Selectable(displayName.c_str(), isSelected)) {
							gmEntry.ScriptPath = gmScripts[i];
							gmEntry.FieldsParsed = false;
							gmEntry.ClassName = "";
							gmEntry.Fields.clear();
						}
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("%s", gmScripts[i].c_str());
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				// Parse fields if not yet done
				if (!gmEntry.ScriptPath.empty() && !gmEntry.FieldsParsed)
					ScriptEngine::ParseScriptFields(gmEntry);

				// Render parsed __fields
				if (!gmEntry.Fields.empty()) {
					ImGui::Separator();
					if (!gmEntry.ClassName.empty())
						ImGui::TextDisabled("%s", gmEntry.ClassName.c_str());
					ImGui::Text("Properties");
					ImGui::Spacing();

					std::string changedFieldName;

					for (auto& field : gmEntry.Fields) {
						std::string fieldLabel = field.Name;
						ImGui::PushID(field.Name.c_str());
						bool changed = false;

						switch (field.Type) {
						case ScriptFieldType::Float:
							{
								float min = (field.Min == 0.0f && field.Max == 0.0f) ? 0.0f : field.Min;
								float max = (field.Min == 0.0f && field.Max == 0.0f) ? 0.0f : field.Max;
								if (ImGui::DragFloat(fieldLabel.c_str(), &field.FloatValue, 0.1f, min, max)) {
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
						case ScriptFieldType::Int:
							{
								int min = (field.Min == 0.0f && field.Max == 0.0f) ? 0 : static_cast<int>(field.Min);
								int max = (field.Min == 0.0f && field.Max == 0.0f) ? 0 : static_cast<int>(field.Max);
								if (ImGui::DragInt(fieldLabel.c_str(), &field.IntValue, 1.0f, min, max)) {
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
						case ScriptFieldType::Bool:
							{
								if (ImGui::Checkbox(fieldLabel.c_str(), &field.BoolValue)) {
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
						case ScriptFieldType::String:
							{
								char buf[256];
								std::strncpy(buf, field.StringValue.c_str(), sizeof(buf) - 1);
								buf[sizeof(buf) - 1] = '\0';
								if (ImGui::InputText(fieldLabel.c_str(), buf, sizeof(buf))) {
									field.StringValue = buf;
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
						case ScriptFieldType::Color:
							{
								if (ImGui::ColorEdit4(fieldLabel.c_str(), &field.ColorValue.x)) {
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
						case ScriptFieldType::Vector3:
							{
								if (ImGui::DragFloat3(fieldLabel.c_str(), &field.Vector3Value.x, 0.1f)) {
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
						}

						if (changed)
							changedFieldName = field.Name;

						ImGui::PopID();
					}

					if (!changedFieldName.empty())
						ScriptEngine::CallOnValidate(gmEntry, changedFieldName);
				}

				ImGui::Spacing();
				ImGui::TextDisabled("Access via: scene:GetGameManager()");
			}

			ComponentCard::End();
		}
	};
}