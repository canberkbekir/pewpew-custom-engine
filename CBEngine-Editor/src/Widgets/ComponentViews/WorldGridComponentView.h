#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Components/WorldGridComponent.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"
#include "CBEngine/Voxel/World/CellularAutomata.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Utils/FileDialogs.h"
#include "../ComponentCard.h"

#include <cstdlib>

namespace CB
{
	class WorldGridComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<WorldGridComponent>())
				return;

			bool removed = false;
			bool reset = false;
			bool opened = ComponentCard::Begin("World Grid", true, &removed, &reset);

			if (removed)
			{
				auto& registry = entity.GetScene()->GetRegistry();
				auto** wgPtr = registry.ctx().find<WorldGrid*>();
				if (wgPtr && *wgPtr)
				{
					delete *wgPtr;
					registry.ctx().erase<WorldGrid*>();
				}
				entity.RemoveComponent<WorldGridComponent>();
				ComponentCard::End();
				return;
			}

			if (reset)
			{
				entity.GetComponent<WorldGridComponent>() = WorldGridComponent{};
			}

			if (!opened)
			{
				ComponentCard::End();
				return;
			}

			auto& comp = entity.GetComponent<WorldGridComponent>();
			auto& registry = entity.GetScene()->GetRegistry();
			auto** wgPtr = registry.ctx().find<WorldGrid*>();
			WorldGrid* grid = (wgPtr && *wgPtr) ? *wgPtr : nullptr;

			size_t currentChunks = grid ? grid->GetChunkCount() : 0;
			size_t expectedChunks = comp.GetExpectedChunkCount();

			// Auto-detect when building finishes
			if (comp.Building && grid && currentChunks >= expectedChunks)
				comp.Building = false;

			// ---- Status + Progress ----
			if (comp.Building)
			{
				float progress = expectedChunks > 0
					? static_cast<float>(currentChunks) / static_cast<float>(expectedChunks)
					: 0.0f;
				if (progress > 1.0f) progress = 1.0f;

				char overlay[64];
				snprintf(overlay, sizeof(overlay), "Building... %zu / %zu chunks",
					currentChunks, expectedChunks);
				ImGui::ProgressBar(progress, ImVec2(-1, 0), overlay);
			}
			else if (!grid || currentChunks == 0)
			{
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No terrain");
			}
			else
			{
				float mb = grid->GetMemoryUsageMB();
				// Count meshed and filled chunks for diagnostics
				size_t meshedCount = 0;
				size_t filledCount = 0;
				for (auto& [coord, chunk] : grid->Chunks())
				{
					if (chunk)
					{
						if (chunk->CachedMesh) meshedCount++;
						if (chunk->FilledCount > 0) filledCount++;
					}
				}
				ImGui::Text("%zu chunks | %zu filled | %zu meshed | %.1f MB",
					currentChunks, filledCount, meshedCount, mb);
			}
			ImGui::Spacing();

			// ---- World Size ----
			{
				ImGui::TextUnformatted("World Size (chunks from origin)");
				ImGui::Separator();

				ImGui::DragInt("Size X##WG", &comp.SizeX, 0.5f, 1, 64);
				ImGui::DragInt("Size Y##WG", &comp.SizeY, 0.5f, 1, 32);
				ImGui::DragInt("Size Z##WG", &comp.SizeZ, 0.5f, 1, 64);

				Vector3 extent = comp.GetWorldExtent();
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
					"%.0f x %.0f x %.0f units  (%zu chunks)",
					extent.x, extent.y, extent.z, expectedChunks);
				ImGui::Spacing();
			}

			// ---- Generation Section ----
			{
				ImGui::TextUnformatted("Generation");
				ImGui::Separator();

				const char* genTypes[] = { "None", "Flat Terrain", "Noise Terrain" };
				ImGui::Combo("Type##WG", &comp.GeneratorType, genTypes, 3);

				if (comp.GeneratorType != 0)
				{
					ImGui::DragScalar("Seed##WG", ImGuiDataType_U32, &comp.Seed, 1.0f);
					ImGui::SameLine();
					if (ImGui::SmallButton("R##WGSeed"))
						comp.Seed = static_cast<uint32_t>(std::rand());

					ImGui::DragInt("Sea Level##WG", &comp.SeaLevel, 1.0f, -256, 256);

					DrawSubstanceCombo("Surface##WG", comp.SurfaceSubstance);
					DrawSubstanceCombo("Fill##WG", comp.FillSubstance);
				}

				if (comp.GeneratorType == 2)
				{
					ImGui::DragFloat("Noise Scale##WG", &comp.NoiseScale, 0.001f, 0.001f, 1.0f, "%.4f");
					ImGui::SliderInt("Octaves##WG", &comp.NoiseOctaves, 1, 8);
					ImGui::DragFloat("Amplitude##WG", &comp.NoiseAmplitude, 0.5f, 1.0f, 256.0f);
				}

				ImGui::Spacing();
			}

			// ---- Action Buttons ----
			{
				float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;

				// Build button
				bool canBuild = (comp.GeneratorType != 0) && !comp.Building;
				if (!canBuild) ImGui::BeginDisabled();

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.4f, 0.15f, 1.0f));
				if (ImGui::Button(comp.Building ? "Building...##WG" : "Build##WG", ImVec2(buttonWidth, 0)))
				{
					if (!grid)
					{
						auto* newGrid = new WorldGrid();
						WorldGrid* ptr = newGrid;
						registry.ctx().insert_or_assign<WorldGrid*>(std::move(ptr));
						grid = newGrid;
					}

					// Clear existing chunks (destroy physics bodies first)
					ClearChunkPhysics(entity.GetScene(), grid);
					grid->Chunks().clear();

					comp.ApplyToGrid(*grid);
					grid->BuildSubstanceLUT();
					grid->BuildGlobalPalette();
					comp.Building = true;
				}
				ImGui::PopStyleColor();

				if (!canBuild) ImGui::EndDisabled();

				ImGui::SameLine();

				// Clear button
				bool hasTerrain = grid && currentChunks > 0 && !comp.Building;
				if (!hasTerrain) ImGui::BeginDisabled();

				bool confirmActive = s_ConfirmClearActive && (ImGui::GetTime() - s_ConfirmClearTime < 2.0);
				if (confirmActive)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
					if (ImGui::Button("Confirm##WGClear", ImVec2(buttonWidth, 0)))
					{
						ClearChunkPhysics(entity.GetScene(), grid);
						grid->Chunks().clear();
						grid->Generator = nullptr;
						grid->GenConfig.GeneratorType = WorldGenConfig::Type::None;
						s_ConfirmClearActive = false;
					}
					ImGui::PopStyleColor();
				}
				else
				{
					if (ImGui::Button("Clear##WG", ImVec2(buttonWidth, 0)))
					{
						s_ConfirmClearActive = true;
						s_ConfirmClearTime = ImGui::GetTime();
					}
				}

				if (!hasTerrain) ImGui::EndDisabled();

				ImGui::SameLine();

				// Save button
				bool canSave = grid && currentChunks > 0 && !comp.Building;
				if (!canSave) ImGui::BeginDisabled();

				if (ImGui::Button("Save##WG", ImVec2(buttonWidth, 0)))
				{
					if (grid)
					{
						if (!comp.StoragePath.empty())
						{
							grid->SetStoragePath(comp.StoragePath);
							grid->FlushAll();
						}
						else
						{
							String path = FileDialogs::SaveFile("World Grid (*.cbwg)\0*.cbwg\0");
							if (!path.empty())
							{
								grid->Save(path);
							}
						}
						s_SaveFeedbackTime = ImGui::GetTime();
					}
				}
				if (s_SaveFeedbackTime > 0.0 && (ImGui::GetTime() - s_SaveFeedbackTime < 1.5))
				{
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Saved!");
				}

				if (!canSave) ImGui::EndDisabled();

				ImGui::Spacing();

				// Save As / Load row
				float halfWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;

				if (!canSave) ImGui::BeginDisabled();
				if (ImGui::Button("Save As...##WG", ImVec2(halfWidth, 0)))
				{
					if (grid)
					{
						String path = FileDialogs::SaveFile("World Grid (*.cbwg)\0*.cbwg\0");
						if (!path.empty())
						{
							grid->Save(path);
							comp.StoragePath = path;
							s_SaveFeedbackTime = ImGui::GetTime();
						}
					}
				}
				if (!canSave) ImGui::EndDisabled();

				ImGui::SameLine();

				if (comp.Building) ImGui::BeginDisabled();
				if (ImGui::Button("Load...##WG", ImVec2(halfWidth, 0)))
				{
					String path = FileDialogs::OpenFile("World Grid (*.cbwg)\0*.cbwg\0");
					if (!path.empty())
					{
						if (!grid)
						{
							auto* newGrid = new WorldGrid();
							WorldGrid* ptr = newGrid;
							registry.ctx().insert_or_assign<WorldGrid*>(std::move(ptr));
							grid = newGrid;
						}

						// Clear existing
						ClearChunkPhysics(entity.GetScene(), grid);
						grid->Chunks().clear();

						comp.ApplyToGrid(*grid);
						grid->BuildSubstanceLUT();
						grid->BuildGlobalPalette();
						grid->Load(path);
						comp.StoragePath = path;
					}
				}
				if (comp.Building) ImGui::EndDisabled();

				// Show current storage path
				if (!comp.StoragePath.empty())
				{
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", comp.StoragePath.c_str());
				}

				ImGui::Spacing();
			}

			// ---- Grid Section ----
			{
				ImGui::TextUnformatted("Grid");
				ImGui::Separator();
				ImGui::DragFloat("Voxel Size##WG", &comp.VoxelSize, 0.01f, 0.05f, 1.0f, "%.3f");
				ImGui::Spacing();
			}

			// ---- Advanced Section ----
			if (ImGui::CollapsingHeader("Advanced##WG"))
			{
				ImGui::DragFloat("Sim Radius##WG", &comp.SimRadius, 1.0f, 16.0f, 1024.0f);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Range for cellular automata simulation");

				ImGui::DragFloat("Render Radius##WG", &comp.RenderRadius, 1.0f, 16.0f, 2048.0f);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Range for mesh rendering");

				ImGui::DragFloat("Physics Radius##WG", &comp.PhysicsRadius, 1.0f, 16.0f, 1024.0f);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Range for collision bodies");

				ImGui::DragFloat("LOD1 Distance##WG", &comp.LOD1Distance, 1.0f, 16.0f, 1024.0f);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance for half-res mesh");

				ImGui::DragFloat("LOD2 Distance##WG", &comp.LOD2Distance, 1.0f, 16.0f, 1024.0f);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance for quarter-res mesh");

				ImGui::DragFloat("Stream Radius##WG", &comp.StreamRadius, 1.0f, 32.0f, 2048.0f);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Range for loading chunks from disk");

				ImGui::DragFloat("Unload Radius##WG", &comp.UnloadRadius, 1.0f, 32.0f, 2048.0f);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Range for unloading chunks from memory");
			}

			ComponentCard::End();
		}

	private:
		static void ClearChunkPhysics(Scene* scene, WorldGrid* grid)
		{
			if (!grid) return;
			PhysicsWorld* physWorld = scene->GetPhysicsWorld();
			if (!physWorld) return;
			for (auto& [coord, chunk] : grid->Chunks())
			{
				if (chunk && chunk->HasPhysicsBody)
				{
					auto& bodyInterface = physWorld->GetBodyInterface();
					bodyInterface.RemoveBody(chunk->PhysicsBodyID);
					bodyInterface.DestroyBody(chunk->PhysicsBodyID);
					chunk->HasPhysicsBody = false;
				}
			}
		}

		static inline bool s_ConfirmClearActive = false;
		static inline double s_ConfirmClearTime = 0.0;
		static inline double s_SaveFeedbackTime = 0.0;

		static void DrawSubstanceCombo(const char* label, std::string& currentSubstance)
		{
			auto allIDs = SubstanceRegistry::GetAllIDs();
			if (allIDs.empty()) { ImGui::TextDisabled("No substances loaded"); return; }

			// Auto-resolve empty or invalid substance to first available
			if (currentSubstance.empty() || !SubstanceRegistry::Exists(currentSubstance))
				currentSubstance = allIDs[0];

			int currentIdx = 0;
			for (int i = 0; i < static_cast<int>(allIDs.size()); i++)
			{
				if (allIDs[i] == currentSubstance)
				{
					currentIdx = i;
					break;
				}
			}

			if (ImGui::BeginCombo(label, allIDs[currentIdx].c_str()))
			{
				for (int i = 0; i < static_cast<int>(allIDs.size()); i++)
				{
					bool isSelected = (i == currentIdx);
					if (ImGui::Selectable(allIDs[i].c_str(), isSelected))
						currentSubstance = allIDs[i];
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
	};
}
