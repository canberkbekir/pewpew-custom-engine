#pragma once

#include "Panel.h"
#include "PewPew/Scene/Entity.h"
#include "PewPew/Scene/Scene.h"

namespace PewPew
{
	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel()
			: Panel("Scene Hierarchy", true) {}
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void OnImGuiRender() override;

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity) { m_SelectionContext = entity; }

	private:
		void DrawEntityNode(Entity entity);
		void DrawCreateEntityMenu();
		void DuplicateEntity(Entity entity);

		Ref<Scene> m_Context;
		Entity m_SelectionContext;
		Entity m_RenamingEntity;
		Entity m_CopiedEntity;

		char m_SearchBuffer[256] = "";
		char m_RenameBuffer[256] = "";
	};
}