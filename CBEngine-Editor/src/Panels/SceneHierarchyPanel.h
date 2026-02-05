// SceneHierarchyPanel.h
#pragma once

#include "Panel.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/Scene.h"

namespace CB
{
	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void OnImGuiRender() override;

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity) { m_SelectionContext = entity; }

	private:
		void DrawCreateEntityMenu();
		void DrawEntityNode(Entity entity);
		void DrawEntityTree();  // New: draws the full hierarchy
		void DuplicateEntity(Entity entity);

		Ref<Scene> m_Context;
		Entity m_SelectionContext;
		Entity m_RenamingEntity;
		Entity m_CopiedEntity;

		char m_SearchBuffer[256] = "";
		char m_RenameBuffer[256] = "";
        
		// Drag and drop state
		Entity m_DraggedEntity;
	};
}