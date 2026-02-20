#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "CBEngine/Utils/FileDialogs.h"
#include "../ComponentCard.h"

namespace CB
{
	class ScriptComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<ScriptComponent>())
				return;

			bool removed = false;
			bool reset = false;
			bool reload = false;

			std::vector<ComponentCard::MenuAction> extraActions;
			extraActions.push_back({"Reload Script", &reload});

			bool opened = ComponentCard::Begin("Script", true, &removed, &reset, extraActions);

			if (removed)
			{
				entity.RemoveComponent<ScriptComponent>();
				ComponentCard::End();
				return;
			}

			auto& script = entity.GetComponent<ScriptComponent>();

			if (reset)
			{
				script.ScriptPath = "";
				script.ClassName = "";
				script.Fields.clear();
				script.ScriptLoaded = false;
				script.FieldsParsed = false;
			}

			if (reload)
			{
				script.FieldsParsed = false;
				if (script.ScriptLoaded)
				{
					script.ScriptLoaded = false;
					ScriptEngine::ReloadScript(script.ScriptPath);
				}
			}

			if (opened)
			{
				// Script path
				char pathBuf[256];
				std::strncpy(pathBuf, script.ScriptPath.c_str(), sizeof(pathBuf) - 1);
				pathBuf[sizeof(pathBuf) - 1] = '\0';

				std::string prevPath = script.ScriptPath;

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

				// Reset fields when script path changes
				if (script.ScriptPath != prevPath)
				{
					script.FieldsParsed = false;
					script.ClassName = "";
					script.Fields.clear();
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
						script.FieldsParsed = false;
						ScriptEngine::ReloadScript(script.ScriptPath);
					}
				}

				// Parse fields if not yet done
				if (!script.ScriptPath.empty() && !script.FieldsParsed)
					ScriptEngine::ParseScriptFields(script);

				// Render script fields
				if (!script.Fields.empty())
				{
					ImGui::Separator();
					if (!script.ClassName.empty())
						ImGui::TextDisabled("%s", script.ClassName.c_str());
					ImGui::Text("Properties");
					ImGui::Spacing();

					std::string changedFieldName;

					for (auto& field : script.Fields)
					{
						std::string label = field.Name;
						ImGui::PushID(field.Name.c_str());
						bool changed = false;

						switch (field.Type)
						{
							case ScriptFieldType::Float:
							{
								float min = (field.Min == 0.0f && field.Max == 0.0f) ? 0.0f : field.Min;
								float max = (field.Min == 0.0f && field.Max == 0.0f) ? 0.0f : field.Max;
								if (ImGui::DragFloat(label.c_str(), &field.FloatValue, 0.1f, min, max))
								{
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
							case ScriptFieldType::Int:
							{
								int min = (field.Min == 0.0f && field.Max == 0.0f) ? 0 : (int)field.Min;
								int max = (field.Min == 0.0f && field.Max == 0.0f) ? 0 : (int)field.Max;
								if (ImGui::DragInt(label.c_str(), &field.IntValue, 1.0f, min, max))
								{
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
							case ScriptFieldType::Bool:
							{
								if (ImGui::Checkbox(label.c_str(), &field.BoolValue))
								{
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
								if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
								{
									field.StringValue = buf;
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
							case ScriptFieldType::Color:
							{
								if (ImGui::ColorEdit4(label.c_str(), &field.ColorValue.x))
								{
									field.HasOverride = true;
									changed = true;
								}
								break;
							}
							case ScriptFieldType::Vector3:
							{
								if (ImGui::DragFloat3(label.c_str(), &field.Vector3Value.x, 0.1f))
								{
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
						ScriptEngine::CallOnValidate(script, changedFieldName);
				}
			}

			ComponentCard::End();
		}
	};
}
