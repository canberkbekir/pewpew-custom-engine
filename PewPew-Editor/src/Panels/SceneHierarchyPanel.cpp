#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "PewPew/Components/CoreComponents.h"
#include "PewPew/Components/TransformComponent.h"
#include "PewPew/Components/MeshRendererComponent.h"
#include "PewPew/Selection/Selection.h"
#include "PewPew/Scene/SceneManager.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace PewPew
{
	// Helper to copy a component if it exists
	template<typename Component>
	static void CopyComponentIfExists(Entity src, Entity dst)
	{
		if (src.HasComponent<Component>())
		{
			auto& srcComponent = src.GetComponent<Component>();
			if (dst.HasComponent<Component>())
				dst.GetComponent<Component>() = srcComponent;
			else
				dst.AddComponent<Component>(srcComponent);
		}
	}

	// Copy all known components from src to dst
	// Add new component types here as you implement them
	static void CopyAllComponents(Entity src, Entity dst)
	{
		// Core components (IDComponent and TagComponent are handled separately)
		CopyComponentIfExists<TransformComponent>(src, dst);
		CopyComponentIfExists<MeshRendererComponent>(src, dst);

		// Add more components as you implement them:
		// CopyComponentIfExists<VoxelDataComponent>(src, dst);
		// CopyComponentIfExists<RigidBodyComponent>(src, dst);
		// CopyComponentIfExists<ColliderComponent>(src, dst);
		// CopyComponentIfExists<DestructibleComponent>(src, dst);
	}

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene)
		: Panel("Scene Hierarchy", true)
	{
		SetContext(scene);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		// Automatically use SceneManager's active scene if no context is set
		if (!m_Context)
		{
			m_Context = SceneManager::GetActiveScene();
		}

		ImGui::Begin(m_Name.c_str(), &m_Visible);

		if (m_Context)
		{
			// Header with entity count
			auto view = m_Context->GetRegistry().view<IDComponent>();
			size_t entityCount = view.size();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();

			// Toolbar
			if (ImGui::Button("+"))
			{
				ImGui::OpenPopup("CreateEntityPopup");
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Create Entity");

			ImGui::SameLine();
			ImGui::TextDisabled("(%d entities)", (int)entityCount);

			// Create entity popup
			if (ImGui::BeginPopup("CreateEntityPopup"))
			{
				DrawCreateEntityMenu();
				ImGui::EndPopup();
			}

			ImGui::PopStyleVar();
			ImGui::Separator();

			// Search filter
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##Search", "Search entities...", m_SearchBuffer, sizeof(m_SearchBuffer));

			ImGui::Separator();

			// Entity list - collect IDs first to avoid iterator invalidation
			std::vector<entt::entity> entities;
			for (auto entityID : view)
			{
				entities.push_back(entityID);
			}

			for (auto entityID : entities)
			{
				Entity entity{ entityID, m_Context.get() };

				// Apply search filter
				if (m_SearchBuffer[0] != '\0')
				{
					String tag = entity.GetComponent<TagComponent>().Tag;
					String lowerTag = tag;
					String lowerSearch = m_SearchBuffer;
					std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
					std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

					if (lowerTag.find(lowerSearch) == String::npos)
						continue;
				}

				DrawEntityNode(entity);
			}

			// Click on empty space to deselect
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
			{
				m_SelectionContext = {};
				Selection::Clear();
			}

			// Right-click on empty space - context menu
			if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				DrawCreateEntityMenu();
				ImGui::EndPopup();
			}

			// Handle keyboard shortcuts
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Delete) && m_SelectionContext)
				{
					Entity toDelete = m_SelectionContext;
					m_SelectionContext = {};
					Selection::Clear();
					m_Context->DestroyEntity(toDelete);
				}

				if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && m_SelectionContext)
				{
					DuplicateEntity(m_SelectionContext);
				}
			}
		}
		else
		{
			ImGui::TextDisabled("No scene loaded");
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawCreateEntityMenu()
	{
		if (ImGui::MenuItem("Empty Entity"))
		{
			Entity entity = m_Context->CreateEntity("Empty Entity");
			m_SelectionContext = entity;
			Selection::Select(Selectable::Entity(entity.GetUUID()));
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("3D Object"))
		{
			if (ImGui::MenuItem("Cube"))
			{
				Entity entity = m_Context->CreateEntity("Cube");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			if (ImGui::MenuItem("Sphere"))
			{
				Entity entity = m_Context->CreateEntity("Sphere");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			if (ImGui::MenuItem("Plane"))
			{
				Entity entity = m_Context->CreateEntity("Plane");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			if (ImGui::MenuItem("Cylinder"))
			{
				Entity entity = m_Context->CreateEntity("Cylinder");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Light"))
		{
			if (ImGui::MenuItem("Point Light"))
			{
				Entity entity = m_Context->CreateEntity("Point Light");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			if (ImGui::MenuItem("Directional Light"))
			{
				Entity entity = m_Context->CreateEntity("Directional Light");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			if (ImGui::MenuItem("Spot Light"))
			{
				Entity entity = m_Context->CreateEntity("Spot Light");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Audio"))
		{
			if (ImGui::MenuItem("Audio Source"))
			{
				Entity entity = m_Context->CreateEntity("Audio Source");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Physics"))
		{
			if (ImGui::MenuItem("Rigidbody"))
			{
				Entity entity = m_Context->CreateEntity("Rigidbody");
				m_SelectionContext = entity;
				Selection::Select(Selectable::Entity(entity.GetUUID()));
			}
			ImGui::EndMenu();
		}
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		// Copy tag string - don't use reference as registry modifications can invalidate it
		String tag = entity.GetComponent<TagComponent>().Tag;
		bool isSelected = m_SelectionContext == entity;
		bool isRenaming = m_RenamingEntity == entity;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_FramePadding;

		if (isSelected)
			flags |= ImGuiTreeNodeFlags_Selected;

		// No children for now, so use leaf flag
		// TODO: Add parent-child relationship support
		flags |= ImGuiTreeNodeFlags_Leaf;

		// Push unique ID (add 1 to avoid 0 which ImGui rejects)
		ImGui::PushID(static_cast<int>((uint32_t)entity + 1));

			// Draw tree node with custom styling
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

		bool opened = ImGui::TreeNodeEx("##EntityNode", flags);

		ImGui::PopStyleVar();

		// Context menu - must be right after TreeNodeEx while it's still the "last item"
		bool entityDeleted = false;
		bool entityDuplicated = false;

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename", "F2"))
			{
				m_RenamingEntity = entity;
				strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
			}

			if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
			{
				entityDuplicated = true;
			}

			if (ImGui::MenuItem("Copy"))
			{
				m_CopiedEntity = entity;
			}

			if (ImGui::MenuItem("Paste", nullptr, false, (bool)m_CopiedEntity))
			{
				if (m_CopiedEntity)
					DuplicateEntity(m_CopiedEntity);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Delete", "Del"))
			{
				entityDeleted = true;
			}

			ImGui::EndPopup();
		}

		// Selection on click
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			m_SelectionContext = entity;
			Selection::Select(Selectable::Entity(entity.GetUUID()));
		}

		// Double-click to rename
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
		{
			m_RenamingEntity = entity;
			strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
			m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
		}

		// Drag source for future parent-child support
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			UUID uuid = entity.GetUUID();
			ImGui::SetDragDropPayload("SCENE_ENTITY", &uuid, sizeof(UUID));
			ImGui::Text("%s", tag.c_str());
			ImGui::EndDragDropSource();
		}

		// Drop target for future parent-child support
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
			{
				// TODO: Implement parent-child relationship
				// UUID droppedUUID = *(UUID*)payload->Data;
				// Entity droppedEntity = m_Context->GetEntityByUUID(droppedUUID);
				// SetParent(droppedEntity, entity);
			}
			ImGui::EndDragDropTarget();
		}

		// Draw label or rename input
		ImGui::SameLine();

		if (isRenaming)
		{
			ImGui::SetKeyboardFocusHere();
			ImGui::PushItemWidth(-1);

			if (ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
			{
				if (m_RenameBuffer[0] != '\0')
					entity.GetComponent<TagComponent>().Tag = m_RenameBuffer;
				m_RenamingEntity = {};
			}

			// Cancel on escape or click elsewhere
			if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
				(!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)))
			{
				m_RenamingEntity = {};
			}

			ImGui::PopItemWidth();
		}
		else
		{
			ImGui::Text("%s", tag.c_str());
		}

		// Handle F2 rename shortcut
		if (isSelected && ImGui::IsKeyPressed(ImGuiKey_F2))
		{
			m_RenamingEntity = entity;
			strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
		}

		ImGui::PopID();

		if (opened)
		{
			// TODO: Draw child entities when parent-child is implemented
			ImGui::TreePop();
		}

		// Handle deferred actions
		if (entityDuplicated)
		{
			DuplicateEntity(entity);
		}

		if (entityDeleted)
		{
			if (m_SelectionContext == entity)
			{
				m_SelectionContext = {};
				Selection::Clear();
			}
			m_Context->DestroyEntity(entity);
		}
	}

	void SceneHierarchyPanel::DuplicateEntity(Entity entity)
	{
		if (!entity || !m_Context)
			return;

		String name = entity.GetName() + " (Copy)";
		Entity newEntity = m_Context->CreateEntity(name);

		// Copy all components generically
		CopyAllComponents(entity, newEntity);

		// Select the new entity
		m_SelectionContext = newEntity;
		Selection::Select(Selectable::Entity(newEntity.GetUUID()));
	}
}
