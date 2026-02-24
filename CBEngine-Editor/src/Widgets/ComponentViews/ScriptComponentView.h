#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "CBEngine/Utils/FileDialogs.h"
#include "../ComponentCard.h"

#include <filesystem>
#include "CBEngine/Asset/AssetManager.h"

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

			bool opened = ComponentCard::Begin("Scripts", true, &removed, &reset);

			if (removed)
			{
				entity.RemoveComponent<ScriptComponent>();
				ComponentCard::End();
				return;
			}

			auto& scriptComp = entity.GetComponent<ScriptComponent>();

			if (reset)
			{
				scriptComp.Scripts.clear();
			}

			if (opened)
			{
				int removeIndex = -1;

				for (size_t i = 0; i < scriptComp.Scripts.size(); i++)
				{
					auto& entry = scriptComp.Scripts[i];
					ImGui::PushID(static_cast<int>(i));

					// Header label: ClassName > filename > "Empty Script"
					std::string label;
					if (!entry.ClassName.empty())
						label = entry.ClassName;
					else if (!entry.ScriptPath.empty())
					{
						std::filesystem::path p(entry.ScriptPath);
						label = p.filename().string();
					}
					else
						label = "Empty Script";

					// Collapsing header with remove button
					bool entryOpen = ImGui::CollapsingHeader(label.c_str(),
						ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);

					// Remove button on the right
					float buttonSize = ImGui::GetFrameHeight();
					ImGui::SameLine(ImGui::GetContentRegionMax().x - buttonSize);
					char removeBtnId[32];
					snprintf(removeBtnId, sizeof(removeBtnId), "X##%d", static_cast<int>(i));
					if (ImGui::Button(removeBtnId, ImVec2(buttonSize, buttonSize)))
						removeIndex = static_cast<int>(i);

					if (entryOpen)
					{
						ImGui::Indent(4.0f);

						// Script path
						char pathBuf[256];
						std::strncpy(pathBuf, entry.ScriptPath.c_str(), sizeof(pathBuf) - 1);
						pathBuf[sizeof(pathBuf) - 1] = '\0';

						std::string prevPath = entry.ScriptPath;

						if (ImGui::InputText("Script Path", pathBuf, sizeof(pathBuf)))
						{
							entry.ScriptPath = pathBuf;
						}

						ImGui::SameLine();
						char browseId[32];
						snprintf(browseId, sizeof(browseId), "...##Browse%d", static_cast<int>(i));
						if (ImGui::Button(browseId))
						{
							std::string path = FileDialogs::OpenFile("Lua Script (*.lua)\0*.lua\0");
							if (!path.empty())
							{
								entry.ScriptPath = path;
								entry.ScriptLoaded = false;
							}
						}

						// Reset fields when script path changes
						if (entry.ScriptPath != prevPath)
						{
							entry.FieldsParsed = false;
							entry.ClassName = "";
							entry.Fields.clear();
						}

						// Status
						if (entry.ScriptLoaded)
						{
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
						}
						else if (!entry.ScriptPath.empty())
						{
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Not loaded (enter play mode)");
						}
						else
						{
							ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No script assigned");
						}

						// Reload button
						if (entry.ScriptLoaded)
						{
							if (ImGui::Button("Reload"))
							{
								entry.ScriptLoaded = false;
								entry.FieldsParsed = false;
								ScriptEngine::ReloadScript(entry.ScriptPath);
							}
						}

						// Parse fields if not yet done
						if (!entry.ScriptPath.empty() && !entry.FieldsParsed)
							ScriptEngine::ParseScriptFields(entry);

						// Render script fields
						if (!entry.Fields.empty())
						{
							ImGui::Separator();
							if (!entry.ClassName.empty())
								ImGui::TextDisabled("%s", entry.ClassName.c_str());
							ImGui::Text("Properties");
							ImGui::Spacing();

							std::string changedFieldName;

							for (auto& field : entry.Fields)
							{
								std::string fieldLabel = field.Name;
								ImGui::PushID(field.Name.c_str());
								bool changed = false;

								switch (field.Type)
								{
									case ScriptFieldType::Float:
									{
										float min = (field.Min == 0.0f && field.Max == 0.0f) ? 0.0f : field.Min;
										float max = (field.Min == 0.0f && field.Max == 0.0f) ? 0.0f : field.Max;
										if (ImGui::DragFloat(fieldLabel.c_str(), &field.FloatValue, 0.1f, min, max))
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
										if (ImGui::DragInt(fieldLabel.c_str(), &field.IntValue, 1.0f, min, max))
										{
											field.HasOverride = true;
											changed = true;
										}
										break;
									}
									case ScriptFieldType::Bool:
									{
										if (ImGui::Checkbox(fieldLabel.c_str(), &field.BoolValue))
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
										if (ImGui::InputText(fieldLabel.c_str(), buf, sizeof(buf)))
										{
											field.StringValue = buf;
											field.HasOverride = true;
											changed = true;
										}
										break;
									}
									case ScriptFieldType::Color:
									{
										if (ImGui::ColorEdit4(fieldLabel.c_str(), &field.ColorValue.x))
										{
											field.HasOverride = true;
											changed = true;
										}
										break;
									}
									case ScriptFieldType::Vector3:
									{
										if (ImGui::DragFloat3(fieldLabel.c_str(), &field.Vector3Value.x, 0.1f))
										{
											field.HasOverride = true;
											changed = true;
										}
										break;
									}
									case ScriptFieldType::EntityRef:
									case ScriptFieldType::ComponentRef:
									case ScriptFieldType::ScriptRef:
									{
										// Determine display label for the assigned entity
										std::string refLabel = "None";
										if (field.EntityRefValue.IsValid())
										{
											Scene* refScene = entity.GetScene();
											if (refScene && refScene->EntityExists(field.EntityRefValue))
											{
												Entity refEntity = refScene->GetEntityByUUID(field.EntityRefValue);
												if (refEntity) refLabel = std::string(refEntity.GetName());
											}
											else { refLabel = "Missing"; }
										}
										if (field.Type != ScriptFieldType::EntityRef && !field.EntityRefSubtype.empty())
											refLabel += " (" + field.EntityRefSubtype + ")";

										// Draw field label + drag-drop target button
										ImGui::Text("%s", fieldLabel.c_str());
										ImGui::SameLine();
										float availW = ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - 4.0f;
										ImGui::Button(refLabel.c_str(), ImVec2(availW, 0));
										if (ImGui::BeginDragDropTarget())
										{
											if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
											{
												uint64_t droppedUUID = *static_cast<const uint64_t*>(payload->Data);
												field.EntityRefValue = UUID(droppedUUID);
												field.HasOverride = true;
												changed = true;
											}
											ImGui::EndDragDropTarget();
										}
										ImGui::SameLine();
										if (ImGui::SmallButton("X"))
										{
											field.EntityRefValue = UUID(0);
											field.EntityRefSubtype = "";
											field.HasOverride = false;
											changed = true;
										}
										break;
									}
									case ScriptFieldType::BlueprintRef:
									{
										std::string bpLabel = "None";
										if (field.EntityRefValue.IsValid())
										{
											if (field.EntityRefSubtype == "blueprint_asset")
											{
												const AssetMetadata* meta = AssetManager::GetRegistry().GetMetadata(field.EntityRefValue);
												bpLabel = meta ? (meta->FilePath.stem().string() + " (Blueprint)") : "Missing Asset";
											}
											else
											{
												Scene* refScene = entity.GetScene();
												if (refScene && refScene->EntityExists(field.EntityRefValue))
												{
													Entity refEntity = refScene->GetEntityByUUID(field.EntityRefValue);
													bpLabel = refEntity ? (std::string(refEntity.GetName()) + " (Entity)") : "Missing Entity";
												}
												else { bpLabel = "Missing Entity"; }
											}
										}
										ImGui::Text("%s", fieldLabel.c_str());
										ImGui::SameLine();
										float availW = ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - 4.0f;
										ImGui::Button(bpLabel.c_str(), ImVec2(availW, 0));
										if (ImGui::BeginDragDropTarget())
										{
											if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
											{
												field.EntityRefValue = UUID(*static_cast<const uint64_t*>(payload->Data));
												field.EntityRefSubtype = "";
												field.HasOverride = true;
												changed = true;
											}
											if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
											{
												std::filesystem::path droppedPath(static_cast<const char*>(payload->Data),
													static_cast<const char*>(payload->Data) + payload->DataSize - 1);
												if (droppedPath.is_relative())
													droppedPath = std::filesystem::absolute(droppedPath);
												if (droppedPath.extension() == ".blueprint")
												{
													UUID assetUUID = AssetManager::ImportAsset(droppedPath);
													if (assetUUID.IsValid())
													{
														field.EntityRefValue = assetUUID;
														field.EntityRefSubtype = "blueprint_asset";
														field.HasOverride = true;
														changed = true;
													}
												}
											}
											ImGui::EndDragDropTarget();
										}
										ImGui::SameLine();
										if (ImGui::SmallButton("X"))
										{
											field.EntityRefValue = UUID(0);
											field.EntityRefSubtype = "";
											field.HasOverride = false;
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
								ScriptEngine::CallOnValidate(entry, changedFieldName);
						}

						ImGui::Unindent(4.0f);
					}

					ImGui::PopID();

					if (i < scriptComp.Scripts.size() - 1)
						ImGui::Separator();
				}

				// Remove entry if requested
				if (removeIndex >= 0 && removeIndex < static_cast<int>(scriptComp.Scripts.size()))
				{
					scriptComp.Scripts.erase(scriptComp.Scripts.begin() + removeIndex);
				}

				ImGui::Spacing();

				// Add Script button
				if (ImGui::Button("+ Add Script", ImVec2(-1, 0)))
				{
					scriptComp.Scripts.push_back(ScriptEntry{});
				}
			}

			ComponentCard::End();
		}
	};
}
