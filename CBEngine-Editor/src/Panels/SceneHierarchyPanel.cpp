// SceneHierarchyPanel.cpp
#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/BlueprintAsset.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/Components.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Selection/Selection.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Utils/FileDialogs.h"
#include "CBEngine/Renderer/Resources/PrimitiveMesh.h"

#include <algorithm>
#include <cstring>
#include <vector>
#include <filesystem>

#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/BlueprintInstanceComponent.h"
#include "CBEngine/Scene/ComponentRegistry.h"

namespace CB
{
	template <typename Component>
	static void CopyComponentIfExists(Entity src,Entity dst)
	{
		if (src.HasComponent<Component>()) {
			auto& srcComponent = src.GetComponent<Component>();
			if (dst.HasComponent<Component>())
				dst.GetComponent<Component>() = srcComponent;
			else
				dst.AddComponent<Component>(srcComponent);
		}
	}

	template <typename... Ts>
	static void CopyComponentsFromList(Entity src, Entity dst, ComponentList<Ts...>)
	{
		(CopyComponentIfExists<Ts>(src, dst), ...);
	}

	static void CopyAllComponents(Entity src,Entity dst)
	{
		CopyComponentsFromList(src, dst, SerializableComponents{});

		// BlueprintInstanceComponent is intentionally excluded from
		// SerializableComponents (handled manually by SceneSerializer)
		CopyComponentIfExists<BlueprintInstanceComponent>(src, dst);
	}

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene)
		: Panel("Scene Hierarchy", true) { SetContext(scene); }

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		Ref<Scene> activeScene = SceneManager::GetActiveScene();
		if (m_Context != activeScene) {
			m_Context = activeScene;
			m_SelectionContext = {};
		}

		// Sync selection from the global Selection system (e.g. viewport picking)
		if (m_Context && Selection::HasEntitySelected()) {
			UUID selectedUUID = Selection::GetPrimarySelection().ID;
			Entity selectedEntity = m_Context->GetEntityByUUID(selectedUUID);
			if (selectedEntity && selectedEntity != m_SelectionContext)
				m_SelectionContext = selectedEntity;
		}
		else if (m_Context && !Selection::HasSelection() && m_SelectionContext) { m_SelectionContext = {}; }

		ImGui::Begin(m_Name.c_str(), &m_Visible);

		if (m_Context) {
			// Scene name header
			const String& sceneName = SceneManager::GetActiveSceneName();
			const String& scenePath = SceneManager::GetActiveScenePath();
			bool modified = SceneManager::IsSceneModified();

			ImGui::Text("%s%s", sceneName.c_str(), modified ? " *" : "");
			if (!scenePath.empty() && ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", scenePath.c_str());

			auto view = m_Context->GetRegistry().view<IDComponent>();
			size_t entityCount = view.size();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
			ImGui::Separator();

			if (ImGui::Button("+"))
				ImGui::OpenPopup("CreateEntityPopup");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Create Entity");

			ImGui::SameLine();
			ImGui::TextDisabled("(%d entities)", static_cast<int>(entityCount));

			if (ImGui::BeginPopup("CreateEntityPopup")) {
				DrawCreateEntityMenu();
				ImGui::EndPopup();
			}

			ImGui::PopStyleVar();
			ImGui::Separator();

			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##Search", "Search entities...", m_SearchBuffer, sizeof(m_SearchBuffer));
			ImGui::Separator();

			// Draw the hierarchy tree
			DrawEntityTree();

			// Drop target for root level (unparent)
			// This creates an invisible drop zone at the bottom
			ImVec2 availRegion = ImGui::GetContentRegionAvail();
			if (availRegion.y > 0) {
				ImGui::InvisibleButton("##RootDropZone", ImVec2(-1, availRegion.y));

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY")) {
						UUID droppedUUID = *static_cast<UUID*>(payload->Data);
						Entity droppedEntity = m_Context->GetEntityByUUID(droppedUUID);
						if (droppedEntity && droppedEntity.HasParent()) { droppedEntity.RemoveParent(); }
					}
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
						std::filesystem::path droppedPath(static_cast<const char*>(payload->Data));
						HandleContentBrowserDrop(droppedPath, {});
					}
					ImGui::EndDragDropTarget();
				}
			}

			// Click on empty space to deselect
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
				m_SelectionContext = {};
				Selection::Clear();
			}

			// Right-click context menu
			if (ImGui::BeginPopupContextWindow(
				nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
				DrawCreateEntityMenu();
				ImGui::EndPopup();
			}

			// Keyboard shortcuts
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
				if (ImGui::IsKeyPressed(ImGuiKey_Delete) && m_SelectionContext) {
					Entity toDelete = m_SelectionContext;
					m_SelectionContext = {};
					Selection::Clear();
					m_Context->DestroyEntity(toDelete);
				}

				if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && m_SelectionContext) {
					DuplicateEntity(m_SelectionContext);
				}
			}
		}
		else { ImGui::TextDisabled("No scene loaded"); }

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityTree()
	{
		// Collect root entities (entities without parents)
		std::vector<Entity> rootEntities;
		auto view = m_Context->GetRegistry().view<IDComponent, TransformComponent>();

		for (auto entityID : view) {
			Entity entity{entityID, m_Context.get()};
			auto& transform = entity.GetComponent<TransformComponent>();

			// Only process root entities here
			if (!transform.HasParent()) {
				// Apply search filter
				if (m_SearchBuffer[0] != '\0') {
					String tag = entity.GetComponent<TagComponent>().Tag;
					String lowerTag = tag;
					String lowerSearch = m_SearchBuffer;
					std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), tolower);
					std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), tolower);

					// When searching, show all matching entities regardless of hierarchy
					// Skip non-matching roots (but their children might still match)
					if (lowerTag.find(lowerSearch) == String::npos) {
						// Still need to check children when searching
						// For now, skip this root if it doesn't match
						continue;
					}
				}

				rootEntities.push_back(entity);
			}
		}

		// Sort by name for consistent ordering
		std::sort(rootEntities.begin(), rootEntities.end(), [](Entity a,Entity b)
		{
			return a.GetComponent<TagComponent>().Tag < b.GetComponent<TagComponent>().Tag;
		});

		// Draw each root entity (which will recursively draw children)
		for (Entity entity : rootEntities) { DrawEntityNode(entity); }
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		String tag = entity.GetComponent<TagComponent>().Tag;
		auto& transform = entity.GetComponent<TransformComponent>();

		bool isSelected = m_SelectionContext == entity;
		bool isRenaming = m_RenamingEntity == entity;
		bool hasChildren = transform.HasChildren();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
		| ImGuiTreeNodeFlags_SpanAvailWidth
		| ImGuiTreeNodeFlags_FramePadding
		| ImGuiTreeNodeFlags_OpenOnDoubleClick;

		if (isSelected)
			flags |= ImGuiTreeNodeFlags_Selected;

		// Leaf node if no children
		if (!hasChildren)
			flags |= ImGuiTreeNodeFlags_Leaf;

		// Use DefaultOpen for better UX
		flags |= ImGuiTreeNodeFlags_DefaultOpen;

		ImGui::PushID(static_cast<int>(static_cast<uint32_t>(entity) + 1));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

		// Add indentation indicator for hierarchy depth
		bool opened = ImGui::TreeNodeEx("##EntityNode", flags);

		ImGui::PopStyleVar();

		// Context menu
		bool entityDeleted = false;
		bool entityDuplicated = false;
		bool unparentEntity = false;

		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Rename", "F2")) {
				m_RenamingEntity = entity;
				strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
			}

			if (ImGui::MenuItem("Duplicate", "Ctrl+D")) { entityDuplicated = true; }

			if (ImGui::MenuItem("Copy")) { m_CopiedEntity = entity; }

			if (ImGui::MenuItem("Paste", nullptr, false, m_CopiedEntity)) {
				if (m_CopiedEntity)
					DuplicateEntity(m_CopiedEntity);
			}

			ImGui::Separator();

			// Hierarchy options
			if (ImGui::BeginMenu("Hierarchy")) {
				if (ImGui::MenuItem("Create Child")) {
					Entity child = m_Context->CreateEntity("Child");
					child.SetParent(entity);
					m_SelectionContext = child;
					Selection::Select(Selectable::Entity(child.GetUUID()));
				}

				if (transform.HasParent() && ImGui::MenuItem("Unparent")) { unparentEntity = true; }

				if (hasChildren && ImGui::MenuItem("Unparent All Children")) {
					auto children = entity.GetChildren();
					for (Entity child : children) { child.RemoveParent(); }
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save as Blueprint")) {
				String yamlData = SceneSerializer::SerializeEntityHierarchy(entity);
				String savePath = FileDialogs::SaveFile("Blueprint (*.blueprint)\0*.blueprint\0");
				if (!savePath.empty()) {
					// Ensure .blueprint extension
					std::filesystem::path path(savePath);
					if (path.extension() != ".blueprint")
						path.replace_extension(".blueprint");

					auto bp = CreateRef<BlueprintAsset>();
					bp->YAMLData = yamlData;
					bp->RootEntityName = tag;
					bp->Save(path);

					// Import if inside assets directory
					auto assetsDir = std::filesystem::path("assets");
					if (path.string().find(assetsDir.string()) != String::npos) {
						std::filesystem::path relPath = relative(path, assetsDir);
						AssetManager::ImportAsset(relPath);
					}
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Delete", "Del")) { entityDeleted = true; }

			ImGui::EndPopup();
		}

		// Selection on click
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			m_SelectionContext = entity;
			Selection::Select(Selectable::Entity(entity.GetUUID()));
		}

		// Double-click to rename (only on leaf or when not toggling)
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !hasChildren) {
			m_RenamingEntity = entity;
			strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
			m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
		}

		// Drag source
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			UUID uuid = entity.GetUUID();
			ImGui::SetDragDropPayload("SCENE_ENTITY", &uuid, sizeof(UUID));
			ImGui::Text("%s", tag.c_str());
			m_DraggedEntity = entity;
			ImGui::EndDragDropSource();
		}

		// Drop target - allow dropping entities to make them children
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY")) {
				UUID droppedUUID = *static_cast<UUID*>(payload->Data);
				Entity droppedEntity = m_Context->GetEntityByUUID(droppedUUID);

				// Prevent dropping on self or creating cycles
				if (droppedEntity && droppedEntity != entity && !entity.IsDescendantOf(droppedEntity)) {
					droppedEntity.SetParent(entity);
				}
			}
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
				std::filesystem::path droppedPath(static_cast<const char*>(payload->Data));
				HandleContentBrowserDrop(droppedPath, entity);
			}
			ImGui::EndDragDropTarget();
		}

		// Draw label or rename input
		ImGui::SameLine();

		// Show hierarchy indicator
		if (transform.HasParent()) {
			ImGui::TextDisabled("|");
			ImGui::SameLine();
		}

		if (isRenaming) {
			ImGui::SetKeyboardFocusHere();
			ImGui::PushItemWidth(-1);

			if (ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer),
			                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
				if (m_RenameBuffer[0] != '\0')
					entity.GetComponent<TagComponent>().Tag = m_RenameBuffer;
				m_RenamingEntity = {};
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
				(!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))) { m_RenamingEntity = {}; }

			ImGui::PopItemWidth();
		}
		else {
			ImGui::Text("%s", tag.c_str());

			// Blueprint instance badge
			if (entity.HasComponent<BlueprintInstanceComponent>()) {
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.3f, 0.5f, 1.0f, 1.0f), "[BP]");
				if (ImGui::IsItemHovered()) {
					auto& bic = entity.GetComponent<BlueprintInstanceComponent>();
					ImGui::SetTooltip("Blueprint: %s", bic.BlueprintPath.c_str());
				}
			}
			else if (entity.HasParent()) {
				// Check if any ancestor has BlueprintInstanceComponent
				Entity ancestor = entity.GetParent();
				while (ancestor) {
					if (ancestor.HasComponent<BlueprintInstanceComponent>()) {
						ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.3f, 0.5f, 1.0f, 0.4f), "[BP]");
						break;
					}
					ancestor = ancestor.HasParent() ? ancestor.GetParent() : Entity{};
				}
			}

			// Show child count if has children
			if (hasChildren) {
				ImGui::SameLine();
				ImGui::TextDisabled("(%d)", static_cast<int>(transform.Children.size()));
			}
		}

		// F2 rename shortcut
		if (isSelected && ImGui::IsKeyPressed(ImGuiKey_F2)) {
			m_RenamingEntity = entity;
			strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
		}

		ImGui::PopID();

		// Draw children recursively
		if (opened) {
			if (hasChildren) {
				// Get and sort children
				std::vector<Entity> children = entity.GetChildren();
				std::sort(children.begin(), children.end(), [](Entity a,Entity b)
				{
					return a.GetComponent<TagComponent>().Tag < b.GetComponent<TagComponent>().Tag;
				});

				for (Entity child : children) {
					// Apply search filter to children too
					if (m_SearchBuffer[0] != '\0') {
						String childTag = child.GetComponent<TagComponent>().Tag;
						String lowerTag = childTag;
						String lowerSearch = m_SearchBuffer;
						std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), tolower);
						std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), tolower);

						if (lowerTag.find(lowerSearch) == String::npos)
							continue;
					}

					DrawEntityNode(child);
				}
			}

			ImGui::TreePop();
		}

		// Handle deferred actions
		if (unparentEntity) { entity.RemoveParent(); }

		if (entityDuplicated) { DuplicateEntity(entity); }

		if (entityDeleted) {
			if (m_SelectionContext == entity) {
				m_SelectionContext = {};
				Selection::Clear();
			}
			m_Context->DestroyEntity(entity);
		}
	}

	void SceneHierarchyPanel::DrawCreateEntityMenu()
	{
		if (ImGui::MenuItem("Empty Entity")) {
			Entity entity = m_Context->CreateEntity("Empty Entity");
			m_SelectionContext = entity;
			Selection::Select(Selectable::Entity(entity.GetUUID()));
		}

		// If an entity is selected, offer to create as child
		if (m_SelectionContext) {
			if (ImGui::MenuItem("Empty Child")) {
				Entity entity = m_Context->CreateEntity("Child");
				entity.SetParent(m_SelectionContext);
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
		}

		if (ImGui::MenuItem("From Blueprint...")) {
			String filePath = FileDialogs::OpenFile("Blueprint (*.blueprint)\0*.blueprint\0");
			if (!filePath.empty()) {
				auto bp = BlueprintAsset::Load(std::filesystem::path(filePath));
				if (bp && m_Context) {
					SceneSerializer serializer(m_Context);
					std::filesystem::path bpPath(filePath);
					std::filesystem::path relPath = relative(bpPath, std::filesystem::path("assets"));
					UUID bpUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
					auto entities = serializer.InstantiateBlueprint(bp->YAMLData, filePath, bpUUID);
					if (!entities.empty()) {
						// Optionally parent to selected entity
						if (m_SelectionContext && !entities[0].HasParent())
							entities[0].SetParent(m_SelectionContext);

						m_SelectionContext = entities[0];
						Selection::Select(Selectable::Entity(entities[0].GetUUID()));
					}
				}
			}
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("3D Object")) {
			// Helper lambda: create entity, add MeshRendererComponent with a primitive mesh,
			// optionally parent to selection, then select it.
			auto spawnPrimitive = [&](const char* name,UUID meshUUID,Ref<Mesh> mesh)
			{
				Entity entity = m_Context->CreateEntity(name);
				auto& mrc = entity.AddComponent<MeshRendererComponent>();
				mrc.MeshUUID = meshUUID;
				mrc.MeshAsset = mesh;
				if (m_SelectionContext)
					entity.SetParent(m_SelectionContext);
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			};

			if (ImGui::MenuItem("Cube"))
				spawnPrimitive("Cube", UUID(PrimitiveUUIDs::Cube), PrimitiveMesh::GetCube());
			if (ImGui::MenuItem("Sphere"))
				spawnPrimitive("Sphere", UUID(PrimitiveUUIDs::Sphere), PrimitiveMesh::GetSphere());
			if (ImGui::MenuItem("Plane"))
				spawnPrimitive("Plane", UUID(PrimitiveUUIDs::Plane), PrimitiveMesh::GetPlane());
			if (ImGui::MenuItem("Cylinder"))
				spawnPrimitive("Cylinder", UUID(PrimitiveUUIDs::Cylinder), PrimitiveMesh::GetCylinder());
			if (ImGui::MenuItem("Capsule"))
				spawnPrimitive("Capsule", UUID(PrimitiveUUIDs::Capsule), PrimitiveMesh::GetCapsule());

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Light")) {
			if (ImGui::MenuItem("Point Light")) {
				Entity entity = m_Context->CreateEntity("Point Light");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			if (ImGui::MenuItem("Directional Light")) {
				Entity entity = m_Context->CreateEntity("Directional Light");
				entity.AddComponent<DirectionalLightComponent>();
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			if (ImGui::MenuItem("Spot Light")) {
				Entity entity = m_Context->CreateEntity("Spot Light");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Audio")) {
			if (ImGui::MenuItem("Audio Source")) {
				Entity entity = m_Context->CreateEntity("Audio Source");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Physics")) {
			if (ImGui::MenuItem("Rigidbody")) {
				Entity entity = m_Context->CreateEntity("Rigidbody");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			ImGui::EndMenu();
		}
	}

	void SceneHierarchyPanel::HandleContentBrowserDrop(const std::filesystem::path& droppedPath,Entity parentEntity)
	{
		if (!m_Context)
			return;

		std::string ext = droppedPath.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

		if (ext == ".blueprint") {
			auto bp = BlueprintAsset::Load(droppedPath);
			if (bp) {
				SceneSerializer serializer(m_Context);
				std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
				UUID bpUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
				auto entities = serializer.InstantiateBlueprint(bp->YAMLData, droppedPath.string(), bpUUID);
				if (!entities.empty()) {
					// If dropping on an entity node, parent the root to it
					if (parentEntity && !entities[0].HasParent())
						entities[0].SetParent(parentEntity);

					m_SelectionContext = entities[0];
					Selection::Select(Selectable::Entity(entities[0].GetUUID()));
				}
			}
		}
		else if (ext == ".mesh") {
			std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
			UUID meshUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
			if (meshUUID.IsValid()) {
				String name = droppedPath.stem().string();
				Entity entity = m_Context->CreateEntity(name);
				auto& mrc = entity.AddComponent<MeshRendererComponent>();
				mrc.MeshUUID = meshUUID;
				mrc.ResolveAssets();
				if (parentEntity)
					entity.SetParent(parentEntity);
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
		}
		else if (ext == ".vmesh") {
			std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
			UUID vmeshUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
			if (vmeshUUID.IsValid()) {
				String name = droppedPath.stem().string();
				Entity entity = m_Context->CreateEntity(name);
				auto& vrc = entity.AddComponent<VoxelRendererComponent>();
				vrc.VoxelMeshUUID = vmeshUUID;
				vrc.ResolveAssets();
				if (parentEntity)
					entity.SetParent(parentEntity);
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
		}
		else if (ext == ".mat" && parentEntity) {
			// Apply material to target entity
			if (parentEntity.HasComponent<MeshRendererComponent>()) {
				std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
				UUID matUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
				if (matUUID.IsValid()) {
					auto& mrc = parentEntity.GetComponent<MeshRendererComponent>();
					mrc.MaterialUUID = matUUID;
					mrc.ResolveAssets();
				}
			}
		}
		else if (ext == ".lua" && parentEntity) {
			// Assign script to entity
			if (!parentEntity.HasComponent<ScriptComponent>())
				parentEntity.AddComponent<ScriptComponent>();
			ScriptEntry entry;
			entry.ScriptPath = droppedPath.string();
			parentEntity.GetComponent<ScriptComponent>().Scripts.push_back(entry);
		}
		else if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") {
			String name = droppedPath.stem().string();
			Entity entity = m_Context->CreateEntity(name);
			entity.AddComponent<MeshRendererComponent>();
			if (parentEntity)
				entity.SetParent(parentEntity);
			m_SelectionContext = entity;
			Selection::Select(Selectable::Entity(entity.GetUUID()));
			CB_CORE_WARN("Dropped raw mesh '{0}' - use Import Panel to process into .mesh first", name);
		}
	}

	void SceneHierarchyPanel::DuplicateEntity(Entity entity)
	{
		if (!entity || !m_Context)
			return;

		String name = entity.GetName() + " (Copy)";
		Entity newEntity = m_Context->CreateEntity(name);

		CopyAllComponents(entity, newEntity);

		// Maintain same parent
		if (entity.HasParent()) { newEntity.SetParent(entity.GetParent()); }

		m_SelectionContext = newEntity;
		Selection::Select(Selectable::Entity(newEntity.GetUUID()));
	}
}