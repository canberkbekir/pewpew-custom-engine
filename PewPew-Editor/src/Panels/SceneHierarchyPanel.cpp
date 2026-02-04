// SceneHierarchyPanel.cpp
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

    static void CopyAllComponents(Entity src, Entity dst)
    {
        CopyComponentIfExists<TransformComponent>(src, dst);
        CopyComponentIfExists<MeshRendererComponent>(src, dst);
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

        Ref<Scene> activeScene = SceneManager::GetActiveScene();
        if (m_Context != activeScene)
        {
            m_Context = activeScene;
            m_SelectionContext = {};
        }

        ImGui::Begin(m_Name.c_str(), &m_Visible);

        if (m_Context)
        {
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
            ImGui::TextDisabled("(%d entities)", (int)entityCount);

            if (ImGui::BeginPopup("CreateEntityPopup"))
            {
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
            if (availRegion.y > 0)
            {
                ImGui::InvisibleButton("##RootDropZone", ImVec2(-1, availRegion.y));
                
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
                    {
                        UUID droppedUUID = *(UUID*)payload->Data;
                        Entity droppedEntity = m_Context->GetEntityByUUID(droppedUUID);
                        if (droppedEntity && droppedEntity.HasParent())
                        {
                            droppedEntity.RemoveParent();
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }

            // Click on empty space to deselect
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                m_SelectionContext = {};
                Selection::Clear();
            }

            // Right-click context menu
            if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                DrawCreateEntityMenu();
                ImGui::EndPopup();
            }

            // Keyboard shortcuts
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

    void SceneHierarchyPanel::DrawEntityTree()
    {
        // Collect root entities (entities without parents)
        std::vector<Entity> rootEntities;
        auto view = m_Context->GetRegistry().view<IDComponent, TransformComponent>();

        for (auto entityID : view)
        {
            Entity entity{ entityID, m_Context.get() };
            auto& transform = entity.GetComponent<TransformComponent>();
            
            // Only process root entities here
            if (!transform.HasParent())
            {
                // Apply search filter
                if (m_SearchBuffer[0] != '\0')
                {
                    String tag = entity.GetComponent<TagComponent>().Tag;
                    String lowerTag = tag;
                    String lowerSearch = m_SearchBuffer;
                    std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
                    std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

                    // When searching, show all matching entities regardless of hierarchy
                    // Skip non-matching roots (but their children might still match)
                    if (lowerTag.find(lowerSearch) == String::npos)
                    {
                        // Still need to check children when searching
                        // For now, skip this root if it doesn't match
                        continue;
                    }
                }

                rootEntities.push_back(entity);
            }
        }

        // Sort by name for consistent ordering
        std::sort(rootEntities.begin(), rootEntities.end(), [](Entity a, Entity b) {
            return a.GetComponent<TagComponent>().Tag < b.GetComponent<TagComponent>().Tag;
        });

        // Draw each root entity (which will recursively draw children)
        for (Entity entity : rootEntities)
        {
            DrawEntityNode(entity);
        }
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

        ImGui::PushID(static_cast<int>((uint32_t)entity + 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        // Add indentation indicator for hierarchy depth
        bool opened = ImGui::TreeNodeEx("##EntityNode", flags);

        ImGui::PopStyleVar();

        // Context menu
        bool entityDeleted = false;
        bool entityDuplicated = false;
        bool unparentEntity = false;

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

            // Hierarchy options
            if (ImGui::BeginMenu("Hierarchy"))
            {
                if (ImGui::MenuItem("Create Child"))
                {
                    Entity child = m_Context->CreateEntity("Child");
                    child.SetParent(entity);
                    m_SelectionContext = child;
                    Selection::Select(Selectable::Entity(child.GetUUID()));
                }

                if (transform.HasParent() && ImGui::MenuItem("Unparent"))
                {
                    unparentEntity = true;
                }

                if (hasChildren && ImGui::MenuItem("Unparent All Children"))
                {
                    auto children = entity.GetChildren();
                    for (Entity child : children)
                    {
                        child.RemoveParent();
                    }
                }

                ImGui::EndMenu();
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

        // Double-click to rename (only on leaf or when not toggling)
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !hasChildren)
        {
            m_RenamingEntity = entity;
            strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
        }

        // Drag source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            UUID uuid = entity.GetUUID();
            ImGui::SetDragDropPayload("SCENE_ENTITY", &uuid, sizeof(UUID));
            ImGui::Text("%s", tag.c_str());
            m_DraggedEntity = entity;
            ImGui::EndDragDropSource();
        }

        // Drop target - allow dropping entities to make them children
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
            {
                UUID droppedUUID = *(UUID*)payload->Data;
                Entity droppedEntity = m_Context->GetEntityByUUID(droppedUUID);
                
                // Prevent dropping on self or creating cycles
                if (droppedEntity && droppedEntity != entity && !entity.IsDescendantOf(droppedEntity))
                {
                    droppedEntity.SetParent(entity);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Draw label or rename input
        ImGui::SameLine();

        // Show hierarchy indicator
        if (transform.HasParent())
        {
            ImGui::TextDisabled("|");
            ImGui::SameLine();
        }

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
            
            // Show child count if has children
            if (hasChildren)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(%d)", (int)transform.Children.size());
            }
        }

        // F2 rename shortcut
        if (isSelected && ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            m_RenamingEntity = entity;
            strncpy_s(m_RenameBuffer, tag.c_str(), sizeof(m_RenameBuffer) - 1);
        }

        ImGui::PopID();

        // Draw children recursively
        if (opened)
        {
            if (hasChildren)
            {
                // Get and sort children
                std::vector<Entity> children = entity.GetChildren();
                std::sort(children.begin(), children.end(), [](Entity a, Entity b) {
                    return a.GetComponent<TagComponent>().Tag < b.GetComponent<TagComponent>().Tag;
                });

                for (Entity child : children)
                {
                    // Apply search filter to children too
                    if (m_SearchBuffer[0] != '\0')
                    {
                        String childTag = child.GetComponent<TagComponent>().Tag;
                        String lowerTag = childTag;
                        String lowerSearch = m_SearchBuffer;
                        std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
                        std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

                        if (lowerTag.find(lowerSearch) == String::npos)
                            continue;
                    }

                    DrawEntityNode(child);
                }
            }

            ImGui::TreePop();
        }

        // Handle deferred actions
        if (unparentEntity)
        {
            entity.RemoveParent();
        }

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

    void SceneHierarchyPanel::DrawCreateEntityMenu()
    {
        if (ImGui::MenuItem("Empty Entity"))
        {
            Entity entity = m_Context->CreateEntity("Empty Entity");
            m_SelectionContext = entity;
            Selection::Select(Selectable::Entity(entity.GetUUID()));
        }

        // If an entity is selected, offer to create as child
        if (m_SelectionContext)
        {
            if (ImGui::MenuItem("Empty Child"))
            {
                Entity entity = m_Context->CreateEntity("Child");
                entity.SetParent(m_SelectionContext);
                m_SelectionContext = entity;
                Selection::Select(Selectable::Entity(entity.GetUUID()));
            }
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("3D Object"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                Entity entity = m_Context->CreateEntity("Cube");
                if (m_SelectionContext)
                    entity.SetParent(m_SelectionContext);
                m_SelectionContext = entity;
                Selection::Select(Selectable::Entity(entity.GetUUID()));
            }
            if (ImGui::MenuItem("Sphere"))
            {
                Entity entity = m_Context->CreateEntity("Sphere");
                if (m_SelectionContext)
                    entity.SetParent(m_SelectionContext);
                m_SelectionContext = entity;
                Selection::Select(Selectable::Entity(entity.GetUUID()));
            }
            if (ImGui::MenuItem("Plane"))
            {
                Entity entity = m_Context->CreateEntity("Plane");
                if (m_SelectionContext)
                    entity.SetParent(m_SelectionContext);
                m_SelectionContext = entity;
                Selection::Select(Selectable::Entity(entity.GetUUID()));
            }
            if (ImGui::MenuItem("Cylinder"))
            {
                Entity entity = m_Context->CreateEntity("Cylinder");
                if (m_SelectionContext)
                    entity.SetParent(m_SelectionContext);
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

    void SceneHierarchyPanel::DuplicateEntity(Entity entity)
    {
        if (!entity || !m_Context)
            return;

        String name = entity.GetName() + " (Copy)";
        Entity newEntity = m_Context->CreateEntity(name);

        CopyAllComponents(entity, newEntity);

        // Maintain same parent
        if (entity.HasParent())
        {
            newEntity.SetParent(entity.GetParent());
        }

        m_SelectionContext = newEntity;
        Selection::Select(Selectable::Entity(newEntity.GetUUID()));
    }
} 