#include "ContentBrowserPanel.h"

#include "imgui.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/Asset.h"
#include "CBEngine/Asset/BlueprintAsset.h"
#include "CBEngine/Selection/Selection.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Events/ApplicationEvent.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "../EditorLayer.h"

#include <algorithm>
#include <fstream>

#ifdef CB_PLATFORM_WINDOWS
#include <Windows.h>
#include <shellapi.h>
#endif

namespace CB
{
	ContentBrowserPanel::ContentBrowserPanel()
		: Panel("Content Browser")
	{
		m_BaseDirectory = "assets";
		m_CurrentDirectory = m_BaseDirectory;

		// Load icons from editor resources
		m_FolderIcon = Texture2D::Create("resources/icons/folder.png");
		m_FileIcon = Texture2D::Create("resources/icons/file.png");

		// Load file type specific icons
		m_FileTypeIcons[".fbx"] = Texture2D::Create("resources/icons/fbx.png");
		m_FileTypeIcons[".obj"] = Texture2D::Create("resources/icons/obj.png");
		m_FileTypeIcons[".gltf"] = Texture2D::Create("resources/icons/gltf.png");
		m_FileTypeIcons[".glb"] = Texture2D::Create("resources/icons/gltf.png");
		m_FileTypeIcons[".glsl"] = Texture2D::Create("resources/icons/shader.png");
		m_FileTypeIcons[".hlsl"] = Texture2D::Create("resources/icons/shader.png");

		// Material icon (uses shader icon as fallback if material.png doesn't exist)
		if (std::filesystem::exists("resources/icons/mat.png"))
			m_FileTypeIcons[".mat"] = Texture2D::Create("resources/icons/mat.png");
		else
			m_FileTypeIcons[".mat"] = m_FileIcon;

		// Processed mesh icons
		if (std::filesystem::exists("resources/icons/mesh.png"))
			m_FileTypeIcons[".mesh"] = Texture2D::Create("resources/icons/mesh.png");
		else
			m_FileTypeIcons[".mesh"] = m_FileIcon;

		if (std::filesystem::exists("resources/icons/vmesh.png"))
			m_FileTypeIcons[".vmesh"] = Texture2D::Create("resources/icons/vmesh.png");
		else
			m_FileTypeIcons[".vmesh"] = m_FileIcon;

		if (std::filesystem::exists("resources/icons/vtex.png"))
			m_FileTypeIcons[".vtex"] = Texture2D::Create("resources/icons/vtex.png");
		else
			m_FileTypeIcons[".vtex"] = m_FileIcon;

		if (std::filesystem::exists("resources/icons/scene.png"))
			m_FileTypeIcons[".scene"] = Texture2D::Create("resources/icons/scene.png");
		else
			m_FileTypeIcons[".scene"] = m_FileIcon;

		// Blueprint icon
		if (std::filesystem::exists("resources/icons/blueprint.png"))
			m_FileTypeIcons[".blueprint"] = Texture2D::Create("resources/icons/blueprint.png");
		else
			m_FileTypeIcons[".blueprint"] = m_FileIcon;

		// Lua script icon
		if (std::filesystem::exists("resources/icons/lua.png"))
			m_FileTypeIcons[".lua"] = Texture2D::Create("resources/icons/lua.png");
		else
			m_FileTypeIcons[".lua"] = m_FileIcon;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::Begin(m_Name.c_str(), &m_Visible);

		// Track hover state for external file drops
		m_IsWindowHovered = ImGui::IsWindowHovered(
			ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

		// Handle keyboard input when window is focused
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) { HandleKeyboardInput(); }

		// Left panel - Directory tree
		ImGui::BeginChild("DirectoryTree", ImVec2(m_TreeWidth, 0), true);
		{
			if (exists(m_BaseDirectory)) { RenderDirectoryTree(m_BaseDirectory); }
		}
		ImGui::EndChild();

		// Drop target for directory tree panel
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
				auto droppedPath = static_cast<const char*>(payload->Data);
				MoveAsset(std::filesystem::path(droppedPath), m_BaseDirectory);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		// Splitter
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::Button("##Splitter", ImVec2(4.0f, -1));
		ImGui::PopStyleColor(3);

		if (ImGui::IsItemActive()) {
			m_TreeWidth += ImGui::GetIO().MouseDelta.x;
			if (m_TreeWidth < 100.0f) m_TreeWidth = 100.0f;
			if (m_TreeWidth > 400.0f) m_TreeWidth = 400.0f;
		}

		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		ImGui::SameLine();

		// Right panel - Content
		ImGui::BeginChild("Content", ImVec2(0, 0), true);
		{
			RenderToolbar();
			RenderBreadcrumbs();

			ImGui::Separator();

			// Right-click on empty space context menu
			RenderContextMenu();

			// Click on empty space to deselect all
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !
				ImGui::IsAnyItemHovered()) {
				if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
					m_SelectedPaths.clear();
					Selection::Clear();
				}

				// Cancel rename if clicking elsewhere
				if (m_IsRenaming) { m_IsRenaming = false; }
			}

			// Calculate how many icons fit per row
			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = static_cast<int>(panelWidth / (m_IconSize + m_Padding));
			columnCount = std::max(columnCount, 1);

			ImGui::Columns(columnCount, nullptr, false);

			// Directory contents (sorted and filtered)
			if (exists(m_CurrentDirectory)) {
				m_CachedEntries = GetSortedEntries();
				m_EntriesDirty = false;

				for (const auto& entry : m_CachedEntries) { RenderContentItem(entry); }
			}
			else { ImGui::TextDisabled("Directory not found: %s", m_CurrentDirectory.string().c_str()); }

			ImGui::Columns(1);

			// Invisible drop zone covering remaining content area space
			ImVec2 remainingRegion = ImGui::GetContentRegionAvail();
			float dropZoneHeight = std::max(remainingRegion.y, 20.0f);
			ImGui::InvisibleButton("##ContentDropZone", ImVec2(-1, dropZoneHeight));

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
					auto droppedPath = static_cast<const char*>(payload->Data);
					std::filesystem::path source(droppedPath);
					if (source.parent_path() != m_CurrentDirectory) { MoveAsset(source, m_CurrentDirectory); }
				}
				// Accept entity drops from Scene Hierarchy -> save as blueprint
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY")) {
					UUID droppedUUID = *static_cast<UUID*>(payload->Data);
					Ref<Scene> scene = SceneManager::GetActiveScene();
					if (scene) {
						Entity entity = scene->GetEntityByUUID(droppedUUID);
						if (entity) {
							String name = entity.GetName();
							String yamlData = SceneSerializer::SerializeEntityHierarchy(entity);

							// Find unique filename
							std::filesystem::path bpPath = m_CurrentDirectory / (name + ".blueprint");
							int counter = 1;
							while (exists(bpPath)) {
								bpPath = m_CurrentDirectory / (name + " " + std::to_string(counter) + ".blueprint");
								counter++;
							}

							auto bp = CreateRef<BlueprintAsset>();
							bp->YAMLData = yamlData;
							bp->RootEntityName = name;
							if (bp->Save(bpPath)) {
								std::filesystem::path relativePath = relative(bpPath, m_BaseDirectory);
								AssetManager::ImportAsset(relativePath);
								m_EntriesDirty = true;
							}
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
		}
		ImGui::EndChild();

		ImGui::End();
	}

	void ContentBrowserPanel::RenderToolbar()
	{
		// Back button
		bool canGoBack = !m_BackHistory.empty();
		if (!canGoBack) ImGui::BeginDisabled();
		if (ImGui::Button("<")) { NavigateBack(); }
		if (!canGoBack) ImGui::EndDisabled();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back");

		ImGui::SameLine();

		// Forward button
		bool canGoForward = !m_ForwardHistory.empty();
		if (!canGoForward) ImGui::BeginDisabled();
		if (ImGui::Button(">")) { NavigateForward(); }
		if (!canGoForward) ImGui::EndDisabled();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Forward");

		ImGui::SameLine();

		// Up button
		bool canGoUp = m_CurrentDirectory != m_BaseDirectory;
		if (!canGoUp) ImGui::BeginDisabled();
		if (ImGui::Button("^")) { NavigateTo(m_CurrentDirectory.parent_path()); }
		if (!canGoUp) ImGui::EndDisabled();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Up");

		ImGui::SameLine();

		// Refresh button
		if (ImGui::Button("Refresh")) { Refresh(); }
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh (F5)");

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Search bar
		ImGui::PushItemWidth(150);
		if (ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer))) {
			m_EntriesDirty = true;
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("X##ClearSearch")) {
			m_SearchBuffer[0] = '\0';
			m_EntriesDirty = true;
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Sort dropdown
		ImGui::Text("Sort:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		const char* sortModes[] = {"Name", "Type", "Date"};
		int currentSort = static_cast<int>(m_SortMode);
		if (ImGui::Combo("##SortMode", &currentSort, sortModes, IM_ARRAYSIZE(sortModes))) {
			m_SortMode = static_cast<SortMode>(currentSort);
			m_EntriesDirty = true;
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button(m_SortAscending ? "Asc" : "Desc")) {
			m_SortAscending = !m_SortAscending;
			m_EntriesDirty = true;
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Type filter dropdown
		ImGui::Text("Filter:");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##TypeFilter", m_ShowAllTypes ? "All" : "Custom")) {
			if (ImGui::Checkbox("All Types", &m_ShowAllTypes)) {
				if (m_ShowAllTypes) {
					m_ShowTextures = m_ShowMeshes = m_ShowRawMeshes = true;
					m_ShowProcessedMeshes = m_ShowShaders = true;
					m_ShowMaterials = m_ShowScenes = true;
					m_ShowBlueprints = m_ShowScripts = m_ShowOther = true;
				}
				m_EntriesDirty = true;
			}
			ImGui::Separator();

			bool anyChanged = false;
			anyChanged |= ImGui::Checkbox("Textures", &m_ShowTextures);
			anyChanged |= ImGui::Checkbox("Meshes", &m_ShowMeshes);
			if (m_ShowMeshes) {
				ImGui::Indent();
				anyChanged |= ImGui::Checkbox("Raw (.fbx, .obj, ...)", &m_ShowRawMeshes);
				anyChanged |= ImGui::Checkbox("Processed (.mesh, .vmesh)", &m_ShowProcessedMeshes);
				ImGui::Unindent();
			}
			anyChanged |= ImGui::Checkbox("Shaders", &m_ShowShaders);
			anyChanged |= ImGui::Checkbox("Materials", &m_ShowMaterials);
			anyChanged |= ImGui::Checkbox("Scenes", &m_ShowScenes);
			anyChanged |= ImGui::Checkbox("Blueprints", &m_ShowBlueprints);
			anyChanged |= ImGui::Checkbox("Scripts", &m_ShowScripts);
			anyChanged |= ImGui::Checkbox("Other", &m_ShowOther);

			if (anyChanged) {
				m_ShowAllTypes = m_ShowTextures && m_ShowMeshes && m_ShowRawMeshes &&
				m_ShowProcessedMeshes && m_ShowShaders &&
				m_ShowMaterials && m_ShowScenes &&
				m_ShowBlueprints && m_ShowScripts && m_ShowOther;
				m_EntriesDirty = true;
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Thumbnail size slider
		ImGui::Text("Size:");
		ImGui::SameLine();
		ImGui::PushItemWidth(80);
		ImGui::SliderFloat("##IconSize", &m_IconSize, 32.0f, 128.0f, "%.0f");
		ImGui::PopItemWidth();
	}

	void ContentBrowserPanel::RenderBreadcrumbs()
	{
		// Build path segments
		std::vector<std::filesystem::path> segments;
		std::filesystem::path current = m_CurrentDirectory;

		while (current != m_BaseDirectory.parent_path() && !current.empty()) {
			segments.push_back(current);
			current = current.parent_path();
		}

		std::reverse(segments.begin(), segments.end());

		// Render clickable breadcrumbs
		for (size_t i = 0; i < segments.size(); i++) {
			const auto& segment = segments[i];
			String name = segment.filename().string();
			if (name.empty()) name = segment.string();

			if (i > 0) {
				ImGui::SameLine();
				ImGui::TextDisabled("/");
				ImGui::SameLine();
			}

			// Make it clickable
			if (segment == m_CurrentDirectory) { ImGui::Text("%s", name.c_str()); }
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
				if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_None,
				                      ImGui::CalcTextSize(name.c_str()))) { NavigateTo(segment); }
				ImGui::PopStyleColor();
			}
		}

		// Selection info
		if (!m_SelectedPaths.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("| %zu selected", m_SelectedPaths.size());
		}
	}

	void ContentBrowserPanel::RenderContextMenu()
	{
		// Context menu on empty space
		if (ImGui::BeginPopupContextWindow("ContentBrowserContextMenu",
		                                   ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			if (ImGui::BeginMenu("Create")) {
				if (ImGui::MenuItem("Folder")) { CreateFolder("New Folder"); }
				if (ImGui::MenuItem("Material")) { CreateMaterial("New Material"); }
				if (ImGui::MenuItem("Scene")) { CreateScene("New Scene"); }
				if (ImGui::MenuItem("Shader")) { CreateShader("New Shader"); }
				if (ImGui::MenuItem("Blueprint")) { CreateBlueprint("New Blueprint"); }
				if (ImGui::MenuItem("Lua Script")) { CreateLuaScript("New Script"); }
				if (ImGui::MenuItem("GameManager Script")) { CreateGameManagerScript("CustomGameManager"); }
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Show in Explorer")) { ShowInExplorer(m_CurrentDirectory); }

			if (ImGui::MenuItem("Refresh", "F5")) { Refresh(); }

			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::HandleKeyboardInput()
	{
		ImGuiIO& io = ImGui::GetIO();

		// Don't process shortcuts when typing in text input
		if (io.WantTextInput)
			return;

		// F5 - Refresh
		if (ImGui::IsKeyPressed(ImGuiKey_F5)) { Refresh(); }

		// F2 - Rename
		if (ImGui::IsKeyPressed(ImGuiKey_F2) && m_SelectedPaths.size() == 1) {
			m_IsRenaming = true;
			m_RenamingPath = *m_SelectedPaths.begin();
			String filename = m_RenamingPath.stem().string();
			strncpy_s(m_RenameBuffer, filename.c_str(), sizeof(m_RenameBuffer) - 1);
		}

		// Delete - Delete selected
		if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_SelectedPaths.empty()) { DeleteSelected(); }

		// Ctrl+A - Select all
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) { SelectAll(); }

		// Ctrl+D - Duplicate
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && !m_SelectedPaths.empty()) { DuplicateSelected(); }

		// Ctrl+C - Copy path
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && m_SelectedPaths.size() == 1) {
			CopyPathToClipboard(*m_SelectedPaths.begin());
		}

		// Backspace - Navigate up
		if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && m_CurrentDirectory != m_BaseDirectory) {
			NavigateTo(m_CurrentDirectory.parent_path());
		}

		// Enter - Open selected folder or file
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) && m_SelectedPaths.size() == 1) {
			const auto& path = *m_SelectedPaths.begin();
			if (is_directory(path)) { NavigateTo(path); }
		}

		// Arrow key navigation
		if (!m_CachedEntries.empty() && (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ||
			ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
			ImGui::IsKeyPressed(ImGuiKey_DownArrow))) {
			int currentIndex = -1;
			if (!m_SelectedPaths.empty()) { currentIndex = GetEntryIndex(*m_SelectedPaths.rbegin()); }

			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = static_cast<int>(panelWidth / (m_IconSize + m_Padding));
			if (columnCount < 1) columnCount = 1;

			int newIndex = currentIndex;

			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
				newIndex = std::max(0, currentIndex - 1);
			else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
				newIndex = std::min(static_cast<int>(m_CachedEntries.size()) - 1, currentIndex + 1);
			else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
				newIndex = std::max(0, currentIndex - columnCount);
			else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
				newIndex = std::min(static_cast<int>(m_CachedEntries.size()) - 1, currentIndex + columnCount);

			if (newIndex >= 0 && newIndex < static_cast<int>(m_CachedEntries.size()) && newIndex != currentIndex) {
				SelectAsset(m_CachedEntries[newIndex].path(), io.KeyCtrl);
			}
		}
	}

	std::vector<std::filesystem::directory_entry> ContentBrowserPanel::GetSortedEntries() const
	{
		std::vector<std::filesystem::directory_entry> entries;

		for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory)) {
			// Skip .meta files
			if (entry.path().extension() == ".meta")
				continue;

			// Apply search filter
			if (!PassesFilter(entry))
				continue;

			// Apply type filter
			if (!PassesTypeFilter(entry))
				continue;

			entries.push_back(entry);
		}

		// Sort entries (directories first, then by selected mode)
		std::sort(entries.begin(), entries.end(),
		          [this](const std::filesystem::directory_entry& a,const std::filesystem::directory_entry& b)
		          {
			          // Directories always come first
			          if (a.is_directory() != b.is_directory())
				          return a.is_directory();

			          int cmp = 0;
			          switch (m_SortMode) {
			          case SortMode::Name:
				          {
					          String nameA = a.path().filename().string();
					          String nameB = b.path().filename().string();
					          std::transform(nameA.begin(), nameA.end(), nameA.begin(), tolower);
					          std::transform(nameB.begin(), nameB.end(), nameB.begin(), tolower);
					          cmp = nameA.compare(nameB);
					          break;
				          }
			          case SortMode::Type:
				          {
					          String extA = a.path().extension().string();
					          String extB = b.path().extension().string();
					          cmp = extA.compare(extB);
					          if (cmp == 0) {
						          String nameA = a.path().filename().string();
						          String nameB = b.path().filename().string();
						          std::transform(nameA.begin(), nameA.end(), nameA.begin(), tolower);
						          std::transform(nameB.begin(), nameB.end(), nameB.begin(), tolower);
						          cmp = nameA.compare(nameB);
					          }
					          break;
				          }
			          case SortMode::DateModified:
				          {
					          auto timeA = a.last_write_time();
					          auto timeB = b.last_write_time();
					          cmp = (timeA < timeB) ? -1 : (timeA > timeB) ? 1 : 0;
					          break;
				          }
			          }

			          return m_SortAscending ? (cmp < 0) : (cmp > 0);
		          });

		return entries;
	}

	bool ContentBrowserPanel::PassesFilter(const std::filesystem::directory_entry& entry) const
	{
		if (m_SearchBuffer[0] == '\0')
			return true;

		String filename = entry.path().filename().string();
		String lowerFilename = filename;
		String lowerSearch = m_SearchBuffer;
		std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), tolower);
		std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), tolower);

		return lowerFilename.find(lowerSearch) != String::npos;
	}

	bool ContentBrowserPanel::PassesTypeFilter(const std::filesystem::directory_entry& entry) const
	{
		if (entry.is_directory())
			return true;

		if (m_ShowAllTypes)
			return true;

		String ext = entry.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".hdr")
			return m_ShowTextures;

		if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
			return m_ShowMeshes && m_ShowRawMeshes;

		if (ext == ".mesh" || ext == ".vmesh")
			return m_ShowMeshes && m_ShowProcessedMeshes;

		if (ext == ".glsl" || ext == ".hlsl" || ext == ".vert" || ext == ".frag" || ext == ".comp")
			return m_ShowShaders;

		if (ext == ".mat" || ext == ".material")
			return m_ShowMaterials;

		if (ext == ".scene" || ext == ".cb")
			return m_ShowScenes;

		if (ext == ".blueprint")
			return m_ShowBlueprints;

		if (ext == ".lua")
			return m_ShowScripts;

		return m_ShowOther;
	}

	void ContentBrowserPanel::Refresh()
	{
		m_ThumbnailCache.clear();
		m_EntriesDirty = true;
		CB_CORE_INFO("ContentBrowser: Refreshed");
	}

	void ContentBrowserPanel::RenderContentItem(const std::filesystem::directory_entry& entry)
	{
		const auto& path = entry.path();
		String filename = path.filename().string();
		bool isSelected = IsAssetSelected(path);
		bool isRenaming = m_IsRenaming && m_RenamingPath == path;

		ImGui::PushID(filename.c_str());

		Ref<Texture2D> icon = GetIconForEntry(entry);

		// Highlight selected items
		if (isSelected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 0.6f));
		else
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		ImGui::ImageButton(
			reinterpret_cast<void*>(static_cast<uint64_t>(icon->GetRendererID())),
			{m_IconSize, m_IconSize},
			{0, 1}, {1, 0}
		);
		ImGui::PopStyleColor();

		// Selection handling
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			if (ImGui::GetIO().KeyShift && !m_LastSelectedPath.empty()) { SelectRange(m_LastSelectedPath, path); }
			else {
				bool addToSelection = ImGui::GetIO().KeyCtrl;
				SelectAsset(path, addToSelection);
			}
		}

		// Drag source
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			m_DraggedItem = path;
			m_IsDragging = true;

			String pathStr = path.string();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);

			ImGui::Image(
				reinterpret_cast<void*>(static_cast<uint64_t>(icon->GetRendererID())),
				ImVec2(32, 32), ImVec2(0, 1), ImVec2(1, 0)
			);
			ImGui::SameLine();
			ImGui::Text("%s", filename.c_str());

			if (m_SelectedPaths.size() > 1)
				ImGui::Text("(+%zu more)", m_SelectedPaths.size() - 1);

			ImGui::EndDragDropSource();
		}
		else { m_IsDragging = false; }

		// Drop target (for folders)
		if (entry.is_directory() && ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
				auto droppedPath = static_cast<const char*>(payload->Data);
				MoveAsset(std::filesystem::path(droppedPath), path);
			}
			ImGui::EndDragDropTarget();
		}

		// Double-click
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			if (entry.is_directory()) { NavigateTo(path); }
			else {
				std::string ext = path.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
				AssetType type = AssetTypeFromExtension(ext);

				// Select the asset for the Properties panel
				std::filesystem::path relPath = relative(path, m_BaseDirectory);
				UUID assetUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
				if (assetUUID.IsValid())
					Selection::Select(Selectable::Asset(assetUUID));

				switch (type) {
				case AssetType::Scene:
					SceneManager::LoadScene(path.string());
					break;
				case AssetType::Material:
					// TODO: Open material in Material Editor
					break;
				case AssetType::Shader:
					// TODO: Open shader in Shader Editor
					break;
				case AssetType::Mesh:
					// TODO: Preview mesh in Viewport
					break;
				case AssetType::ProcessedMesh:
				case AssetType::VoxelMesh:
					// Select to show in Properties panel (already done above)
					break;
				case AssetType::VoxelTexture:
					if (assetUUID.IsValid())
						EditorLayer::RequestOpenVoxelTexture(assetUUID);
					break;
				case AssetType::Texture2D:
					// TODO: Open texture in Texture Viewer
					break;
				case AssetType::Blueprint:
					// Select to show in Properties panel (already done above)
					break;
				case AssetType::Script:
					// Select to show in Properties panel (already done above)
					break;
				default:
					break;
				}
			}
		}

		// Right-click context menu
		if (ImGui::BeginPopupContextItem()) {
			if (entry.is_directory()) { if (ImGui::MenuItem("Open")) { NavigateTo(path); } }

			if (ImGui::MenuItem("Rename", "F2")) {
				m_IsRenaming = true;
				m_RenamingPath = path;
				String stem = path.stem().string();
				strncpy_s(m_RenameBuffer, stem.c_str(), sizeof(m_RenameBuffer) - 1);
			}

			if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
				if (m_SelectedPaths.empty() || m_SelectedPaths.find(path) == m_SelectedPaths.end()) {
					SelectAsset(path, false);
				}
				DuplicateSelected();
			}

			if (ImGui::MenuItem("Delete", "Delete")) {
				if (m_SelectedPaths.empty() || m_SelectedPaths.find(path) == m_SelectedPaths.end()) {
					SelectAsset(path, false);
				}
				DeleteSelected();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Copy Path", "Ctrl+C")) { CopyPathToClipboard(path); }

			if (ImGui::MenuItem("Show in Explorer")) { ShowInExplorer(path); }

			if (!entry.is_directory()) {
				if (ImGui::MenuItem("Reimport")) { ReimportAsset(path); }

				// Instantiate blueprint in scene
				std::string ctxExt = path.extension().string();
				std::transform(ctxExt.begin(), ctxExt.end(), ctxExt.begin(), tolower);
				if (ctxExt == ".blueprint" && ImGui::MenuItem("Instantiate in Scene")) {
					std::filesystem::path relPath = relative(path, m_BaseDirectory);
					UUID bpUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
					if (bpUUID.IsValid()) {
						auto bpAsset = AssetManager::GetAsset<BlueprintAsset>(bpUUID);
						if (bpAsset) {
							Ref<Scene> scene = SceneManager::GetActiveScene();
							if (scene) {
								SceneSerializer serializer(scene);
								serializer.InstantiateBlueprint(bpAsset->YAMLData, path.string(), bpUUID);
							}
						}
					}
					else {
						// Load directly from file if not in registry
						auto bpAsset = BlueprintAsset::Load(path);
						if (bpAsset) {
							Ref<Scene> scene = SceneManager::GetActiveScene();
							if (scene) {
								SceneSerializer serializer(scene);
								serializer.InstantiateBlueprint(bpAsset->YAMLData, path.string());
							}
						}
					}
				}

				// Generate VTexture from .vmesh
				ctxExt = path.extension().string();
				std::transform(ctxExt.begin(), ctxExt.end(), ctxExt.begin(), tolower);
				if (ctxExt == ".vmesh" && ImGui::MenuItem("Generate VTexture")) {
					std::filesystem::path relPath = relative(path, m_BaseDirectory);
					UUID vmeshUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
					if (vmeshUUID.IsValid()) {
						auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vmeshUUID);
						if (vmesh) {
							auto vtex = VoxelTextureAsset::GenerateFromVmesh(vmesh);
							if (vtex) {
								std::filesystem::path vtexPath = path;
								vtexPath.replace_extension(".vtex");
								if (vtex->Save(vtexPath)) {
									std::filesystem::path relVtex = relative(
										vtexPath, AssetManager::GetAssetDirectory());
									UUID vtexUUID = AssetManager::ImportAsset(relVtex);
									if (vtexUUID.IsValid())
										EditorLayer::RequestOpenVoxelTexture(vtexUUID);
								}
							}
						}
					}
				}
			}

			ImGui::EndPopup();
		}

		// Filename text or rename input
		if (isRenaming) {
			ImGui::SetKeyboardFocusHere();
			ImGui::PushItemWidth(m_IconSize);
			if (ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer),
			                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
				RenameAsset(m_RenamingPath, m_RenameBuffer);
				m_IsRenaming = false;
			}
			ImGui::PopItemWidth();

			// Cancel on Escape
			if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { m_IsRenaming = false; }
		}
		else { ImGui::TextWrapped("%s", filename.c_str()); }

		ImGui::NextColumn();
		ImGui::PopID();
	}

	void ContentBrowserPanel::RenderDirectoryTree(const std::filesystem::path& directory)
	{
		String dirName = directory.filename().string();
		if (dirName.empty())
			dirName = directory.string();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

		if (m_CurrentDirectory == directory)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool hasSubDirs = false;
		if (exists(directory)) {
			for (const auto& entry : std::filesystem::directory_iterator(directory)) {
				if (entry.is_directory()) {
					hasSubDirs = true;
					break;
				}
			}
		}

		if (!hasSubDirs)
			flags |= ImGuiTreeNodeFlags_Leaf;

		ImGui::Image(
			reinterpret_cast<void*>(static_cast<uint64_t>(m_FolderIcon->GetRendererID())),
			ImVec2(16, 16), ImVec2(0, 1), ImVec2(1, 0)
		);
		ImGui::SameLine();

		bool opened = ImGui::TreeNodeEx(dirName.c_str(), flags);

		// Click to navigate
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) { NavigateTo(directory); }

		// Drop target for tree nodes
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
				auto droppedPath = static_cast<const char*>(payload->Data);
				MoveAsset(std::filesystem::path(droppedPath), directory);
			}
			ImGui::EndDragDropTarget();
		}

		if (opened) {
			if (exists(directory)) {
				for (const auto& entry : std::filesystem::directory_iterator(directory)) {
					if (entry.is_directory()) { RenderDirectoryTree(entry.path()); }
				}
			}
			ImGui::TreePop();
		}
	}

	Ref<Texture2D> ContentBrowserPanel::GetIconForEntry(const std::filesystem::directory_entry& entry)
	{
		if (entry.is_directory())
			return m_FolderIcon;

		String extension = entry.path().extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

		if (IsImageFile(extension)) {
			String pathStr = entry.path().string();
			auto it = m_ThumbnailCache.find(pathStr);
			if (it != m_ThumbnailCache.end())
				return it->second;

			Ref<Texture2D> thumbnail = Texture2D::Create(pathStr);
			m_ThumbnailCache[pathStr] = thumbnail;
			return thumbnail;
		}

		auto it = m_FileTypeIcons.find(extension);
		if (it != m_FileTypeIcons.end())
			return it->second;

		return m_FileIcon;
	}

	bool ContentBrowserPanel::IsImageFile(const String& extension) const
	{
		return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
		extension == ".bmp" || extension == ".tga";
	}

	void ContentBrowserPanel::SelectAsset(const std::filesystem::path& path,bool addToSelection)
	{
		std::filesystem::path relativePath = relative(path, m_BaseDirectory);
		UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

		if (addToSelection) {
			if (m_SelectedPaths.count(path) > 0) {
				m_SelectedPaths.erase(path);
				if (uuid.IsValid())
					Selection::RemoveFromSelection(Selectable::Asset(uuid));
			}
			else {
				m_SelectedPaths.insert(path);
				if (uuid.IsValid())
					Selection::AddToSelection(Selectable::Asset(uuid));
			}
		}
		else {
			m_SelectedPaths.clear();
			m_SelectedPaths.insert(path);

			if (uuid.IsValid())
				Selection::Select(Selectable::Asset(uuid));
			else
				Selection::Clear();
		}

		m_LastSelectedPath = path;
	}

	void ContentBrowserPanel::SelectRange(const std::filesystem::path& from,const std::filesystem::path& to)
	{
		int fromIndex = GetEntryIndex(from);
		int toIndex = GetEntryIndex(to);

		if (fromIndex < 0 || toIndex < 0)
			return;

		int start = std::min(fromIndex, toIndex);
		int end = std::max(fromIndex, toIndex);

		m_SelectedPaths.clear();
		Selection::Clear();

		for (int i = start; i <= end; i++) {
			const auto& entry = m_CachedEntries[i];
			m_SelectedPaths.insert(entry.path());

			std::filesystem::path relativePath = relative(entry.path(), m_BaseDirectory);
			UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);
			if (uuid.IsValid())
				Selection::AddToSelection(Selectable::Asset(uuid));
		}

		m_LastSelectedPath = to;
	}

	void ContentBrowserPanel::SelectAll()
	{
		m_SelectedPaths.clear();
		Selection::Clear();

		for (const auto& entry : m_CachedEntries) {
			m_SelectedPaths.insert(entry.path());

			std::filesystem::path relativePath = relative(entry.path(), m_BaseDirectory);
			UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);
			if (uuid.IsValid())
				Selection::AddToSelection(Selectable::Asset(uuid));
		}
	}

	bool ContentBrowserPanel::IsAssetSelected(const std::filesystem::path& path) const
	{
		return m_SelectedPaths.count(path) > 0;
	}

	int ContentBrowserPanel::GetEntryIndex(const std::filesystem::path& path) const
	{
		for (size_t i = 0; i < m_CachedEntries.size(); i++) {
			if (m_CachedEntries[i].path() == path)
				return static_cast<int>(i);
		}
		return -1;
	}

	// Navigation
	void ContentBrowserPanel::NavigateTo(const std::filesystem::path& directory)
	{
		if (directory == m_CurrentDirectory)
			return;

		m_BackHistory.push_back(m_CurrentDirectory);
		m_ForwardHistory.clear();
		m_CurrentDirectory = directory;
		m_SelectedPaths.clear();
		Selection::Clear();
		m_EntriesDirty = true;
	}

	void ContentBrowserPanel::NavigateBack()
	{
		if (m_BackHistory.empty())
			return;

		m_ForwardHistory.push_back(m_CurrentDirectory);
		m_CurrentDirectory = m_BackHistory.back();
		m_BackHistory.pop_back();
		m_SelectedPaths.clear();
		Selection::Clear();
		m_EntriesDirty = true;
	}

	void ContentBrowserPanel::NavigateForward()
	{
		if (m_ForwardHistory.empty())
			return;

		m_BackHistory.push_back(m_CurrentDirectory);
		m_CurrentDirectory = m_ForwardHistory.back();
		m_ForwardHistory.pop_back();
		m_SelectedPaths.clear();
		Selection::Clear();
		m_EntriesDirty = true;
	}

	// File Operations
	void ContentBrowserPanel::DeleteSelected()
	{
		for (const auto& path : m_SelectedPaths) {
			std::filesystem::path relativePath = relative(path, m_BaseDirectory);
			UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

			if (uuid.IsValid()) { AssetManager::DeleteAsset(uuid); }
			else {
				// Delete file directly if not in registry
				std::error_code ec;
				remove_all(path, ec);

				// Also remove .meta file
				std::filesystem::path metaPath = path.string() + ".meta";
				if (exists(metaPath))
					std::filesystem::remove(metaPath, ec);
			}

			CB_CORE_INFO("Deleted: {0}", path.string());
		}

		m_SelectedPaths.clear();
		Selection::Clear();
		m_EntriesDirty = true;
	}

	void ContentBrowserPanel::DuplicateSelected()
	{
		std::vector<std::filesystem::path> pathsToDuplicate(m_SelectedPaths.begin(), m_SelectedPaths.end());

		for (const auto& path : pathsToDuplicate) {
			if (is_directory(path))
				continue; // Skip directories for now

			std::filesystem::path newPath = path;
			String stem = path.stem().string();
			String ext = path.extension().string();

			// Find unique name
			int counter = 1;
			while (exists(newPath)) {
				newPath = path.parent_path() / (stem + "_" + std::to_string(counter) + ext);
				counter++;
			}

			std::error_code ec;
			copy_file(path, newPath, ec);

			if (!ec) {
				// Import the duplicated asset
				std::filesystem::path relativePath = relative(newPath, m_BaseDirectory);
				AssetManager::ImportAsset(relativePath);
				CB_CORE_INFO("Duplicated: {0} -> {1}", path.string(), newPath.string());
			}
		}

		m_EntriesDirty = true;
	}

	void ContentBrowserPanel::RenameAsset(const std::filesystem::path& path,const std::string& newName)
	{
		if (newName.empty())
			return;

		std::filesystem::path newPath = path.parent_path() / (newName + path.extension().string());

		if (newPath == path)
			return;

		if (exists(newPath)) {
			CB_CORE_WARN("Cannot rename: file already exists: {0}", newPath.string());
			return;
		}

		// Get UUID before renaming
		std::filesystem::path relativePath = relative(path, m_BaseDirectory);
		UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

		std::error_code ec;
		std::filesystem::rename(path, newPath, ec);

		if (ec) {
			CB_CORE_ERROR("Failed to rename: {0}", ec.message());
			return;
		}

		// Rename .meta file
		std::filesystem::path oldMetaPath = path.string() + ".meta";
		std::filesystem::path newMetaPath = newPath.string() + ".meta";
		if (exists(oldMetaPath)) { std::filesystem::rename(oldMetaPath, newMetaPath, ec); }

		// Update registry
		if (uuid.IsValid()) {
			std::filesystem::path newRelativePath = relative(newPath, m_BaseDirectory);
			AssetManager::GetRegistry().UpdatePath(uuid, newRelativePath);
		}

		// Update selection
		m_SelectedPaths.erase(path);
		m_SelectedPaths.insert(newPath);
		m_LastSelectedPath = newPath;

		CB_CORE_INFO("Renamed: {0} -> {1}", path.filename().string(), newPath.filename().string());
		m_EntriesDirty = true;
	}

	void ContentBrowserPanel::MoveAsset(const std::filesystem::path& source,const std::filesystem::path& destFolder)
	{
		if (!is_directory(destFolder))
			return;

		std::filesystem::path dest = destFolder / source.filename();

		if (dest == source)
			return;

		if (exists(dest)) {
			CB_CORE_WARN("Cannot move: file already exists: {0}", dest.string());
			return;
		}

		std::filesystem::path relativePath = relative(source, m_BaseDirectory);
		UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

		std::error_code ec;
		std::filesystem::rename(source, dest, ec);

		if (ec) {
			CB_CORE_ERROR("Failed to move: {0}", ec.message());
			return;
		}

		// Move .meta file
		std::filesystem::path oldMetaPath = source.string() + ".meta";
		std::filesystem::path newMetaPath = dest.string() + ".meta";
		if (exists(oldMetaPath)) { std::filesystem::rename(oldMetaPath, newMetaPath, ec); }

		// Update registry
		if (uuid.IsValid()) {
			std::filesystem::path newRelativePath = relative(dest, m_BaseDirectory);
			AssetManager::GetRegistry().UpdatePath(uuid, newRelativePath);
		}

		m_SelectedPaths.erase(source);
		CB_CORE_INFO("Moved: {0} -> {1}", source.string(), dest.string());
		m_EntriesDirty = true;
	}

	void ContentBrowserPanel::CreateFolder(const std::string& name)
	{
		std::filesystem::path folderPath = m_CurrentDirectory / name;

		int counter = 1;
		while (exists(folderPath)) {
			folderPath = m_CurrentDirectory / (name + " " + std::to_string(counter));
			counter++;
		}

		std::error_code ec;
		create_directory(folderPath, ec);

		if (!ec) {
			CB_CORE_INFO("Created folder: {0}", folderPath.string());
			m_EntriesDirty = true;

			// Start renaming the new folder
			m_IsRenaming = true;
			m_RenamingPath = folderPath;
			strncpy_s(m_RenameBuffer, folderPath.filename().string().c_str(), sizeof(m_RenameBuffer) - 1);
		}
	}

	void ContentBrowserPanel::CreateMaterial(const std::string& name)
	{
		std::filesystem::path materialPath = m_CurrentDirectory / (name + ".mat");

		int counter = 1;
		while (exists(materialPath)) {
			materialPath = m_CurrentDirectory / (name + " " + std::to_string(counter) + ".mat");
			counter++;
		}

		// Create default material using Material class
		auto material = Material::Create();
		material->SetAlbedo({1.0f, 1.0f, 1.0f});
		material->SetMetallic(0.0f);
		material->SetRoughness(0.5f);
		material->SetSmoothShading(1.0f);

		if (material->Save(materialPath.string())) {
			// Import the material
			std::filesystem::path relativePath = relative(materialPath, m_BaseDirectory);
			AssetManager::ImportAsset(relativePath);

			CB_CORE_INFO("Created material: {0}", materialPath.string());
			m_EntriesDirty = true;

			// Start renaming
			m_IsRenaming = true;
			m_RenamingPath = materialPath;
			strncpy_s(m_RenameBuffer, name.c_str(), sizeof(m_RenameBuffer) - 1);
		}
	}

	void ContentBrowserPanel::CreateScene(const std::string& name)
	{
		std::filesystem::path scenePath = m_CurrentDirectory / (name + ".scene");

		int counter = 1;
		while (exists(scenePath)) {
			scenePath = m_CurrentDirectory / (name + " " + std::to_string(counter) + ".scene");
			counter++;
		}

		// Create an empty scene file using SceneSerializer
		auto scene = CreateRef<Scene>();
		SceneSerializer serializer(scene);
		serializer.SerializeText(scenePath.string());

		// Import the scene asset
		std::filesystem::path relativePath = relative(scenePath, m_BaseDirectory);
		AssetManager::ImportAsset(relativePath);

		CB_CORE_INFO("Created scene: {0}", scenePath.string());
		m_EntriesDirty = true;

		// Start renaming
		m_IsRenaming = true;
		m_RenamingPath = scenePath;
		strncpy_s(m_RenameBuffer, name.c_str(), sizeof(m_RenameBuffer) - 1);
	}

	// Context Menu Actions
	void ContentBrowserPanel::ShowInExplorer(const std::filesystem::path& path)
	{
#ifdef CB_PLATFORM_WINDOWS
		std::filesystem::path absolutePath = absolute(path);

		if (is_directory(absolutePath)) {
			ShellExecuteA(nullptr, "explore", absolutePath.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		}
		else {
			// Select the file in explorer
			String command = "/select,\"" + absolutePath.string() + "\"";
			ShellExecuteA(nullptr, nullptr, "explorer.exe", command.c_str(), nullptr, SW_SHOWNORMAL);
		}
#endif
	}

	void ContentBrowserPanel::CopyPathToClipboard(const std::filesystem::path& path)
	{
		std::filesystem::path absolutePath = absolute(path);
		ImGui::SetClipboardText(absolutePath.string().c_str());
		CB_CORE_INFO("Copied path to clipboard: {0}", absolutePath.string());
	}

	void ContentBrowserPanel::ReimportAsset(const std::filesystem::path& path)
	{
		std::filesystem::path relativePath = relative(path, m_BaseDirectory);
		UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

		// Check if this is a processed mesh type — route through MeshImportPanel
		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
		if ((ext == ".mesh" || ext == ".vmesh") && uuid.IsValid()) {
			EditorLayer::RequestImportPreviewReimport(path, uuid);
			CB_CORE_INFO("Queued reimport via MeshImportPanel: {0}", path.string());
		}
		else if (uuid.IsValid()) {
			AssetManager::QueueReload(uuid);
			CB_CORE_INFO("Queued reimport: {0}", path.string());
		}

		// Clear thumbnail cache for this file
		m_ThumbnailCache.erase(path.string());
	}

	void ContentBrowserPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<FileDropEvent>(CB_BIND_EVENT_FN(ContentBrowserPanel::OnFileDrop));
	}

	bool ContentBrowserPanel::OnFileDrop(FileDropEvent& e)
	{
		// Only handle drops when the Content Browser panel is hovered
		if (!m_IsWindowHovered || !m_Visible)
			return false;

		ImportExternalFiles(e.GetPaths());
		return true;
	}

	void ContentBrowserPanel::CreateShader(const std::string& name)
	{
		std::filesystem::path shaderPath = m_CurrentDirectory / (name + ".glsl");

		int counter = 1;
		while (exists(shaderPath)) {
			shaderPath = m_CurrentDirectory / (name + " " + std::to_string(counter) + ".glsl");
			counter++;
		}

		std::ofstream fout(shaderPath);
		if (fout.is_open()) {
			fout << "#type vertex\n";
			fout << "#version 450 core\n\n";
			fout << "layout(location = 0) in vec3 a_Position;\n";
			fout << "layout(location = 1) in vec3 a_Normal;\n";
			fout << "layout(location = 2) in vec2 a_TexCoords;\n\n";
			fout << "uniform mat4 u_ViewProjection;\n";
			fout << "uniform mat4 u_Transform;\n\n";
			fout << "out vec3 v_Normal;\n";
			fout << "out vec2 v_TexCoords;\n\n";
			fout << "void main()\n";
			fout << "{\n";
			fout << "    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);\n";
			fout << "    v_Normal = mat3(u_Transform) * a_Normal;\n";
			fout << "    v_TexCoords = a_TexCoords;\n";
			fout << "}\n\n";
			fout << "#type fragment\n";
			fout << "#version 450 core\n\n";
			fout << "layout(location = 0) out vec4 o_Color;\n\n";
			fout << "in vec3 v_Normal;\n";
			fout << "in vec2 v_TexCoords;\n\n";
			fout << "void main()\n";
			fout << "{\n";
			fout << "    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));\n";
			fout << "    float diff = max(dot(normalize(v_Normal), lightDir), 0.0);\n";
			fout << "    o_Color = vec4(vec3(0.1 + diff * 0.9), 1.0);\n";
			fout << "}\n";
			fout.close();

			std::filesystem::path relativePath = relative(shaderPath, m_BaseDirectory);
			AssetManager::ImportAsset(relativePath);

			CB_CORE_INFO("Created shader: {0}", shaderPath.string());
			m_EntriesDirty = true;

			m_IsRenaming = true;
			m_RenamingPath = shaderPath;
			strncpy_s(m_RenameBuffer, name.c_str(), sizeof(m_RenameBuffer) - 1);
		}
	}

	void ContentBrowserPanel::CreateBlueprint(const std::string& name)
	{
		std::filesystem::path bpPath = m_CurrentDirectory / (name + ".blueprint");

		int counter = 1;
		while (exists(bpPath)) {
			bpPath = m_CurrentDirectory / (name + " " + std::to_string(counter) + ".blueprint");
			counter++;
		}

		// Create an empty blueprint file
		std::ofstream fout(bpPath);
		if (fout.is_open()) {
			fout << "Blueprint: " << name << "\n";
			fout << "Entities:\n";
			fout.close();

			std::filesystem::path relativePath = relative(bpPath, m_BaseDirectory);
			AssetManager::ImportAsset(relativePath);

			CB_CORE_INFO("Created blueprint: {0}", bpPath.string());
			m_EntriesDirty = true;

			m_IsRenaming = true;
			m_RenamingPath = bpPath;
			strncpy_s(m_RenameBuffer, name.c_str(), sizeof(m_RenameBuffer) - 1);
		}
	}

	// Convert a name like "player controller" to PascalCase "PlayerController"
	static std::string SanitizeLuaClassName(const std::string& name)
	{
		std::string result;
		bool capitalizeNext = true;
		for (char c : name) {
			if (c == ' ' || c == '-' || c == '_') {
				capitalizeNext = true;
				continue;
			}
			if (capitalizeNext) {
				result += static_cast<char>(toupper(c));
				capitalizeNext = false;
			}
			else { result += c; }
		}
		// Ensure starts with letter
		if (!result.empty() && isdigit(result[0]))
			result = "Script" + result;
		if (result.empty())
			result = "NewScript";
		return result;
	}

	void ContentBrowserPanel::CreateLuaScript(const std::string& name)
	{
		std::filesystem::path scriptPath = m_CurrentDirectory / (name + ".lua");

		int counter = 1;
		while (exists(scriptPath)) {
			scriptPath = m_CurrentDirectory / (name + " " + std::to_string(counter) + ".lua");
			counter++;
		}

		std::string className = SanitizeLuaClassName(name);

		std::ofstream fout(scriptPath);
		if (fout.is_open()) {
			fout << "-- " << name << ".lua\n\n";
			fout << className << " = {\n";
			fout << "    __fields = {\n";
			fout << "        -- Speed = Float(5.0, 0, 100),\n";
			fout << "        -- Health = Int(100),\n";
			fout << "    }\n";
			fout << "}\n\n";
			fout << "function " << className << ":OnCreate()\n";
			fout << "    -- Called when the entity is created\n";
			fout << "end\n\n";
			fout << "function " << className << ":OnUpdate(dt)\n";
			fout << "    -- Called every frame\n";
			fout << "    -- dt = delta time in seconds\n";
			fout << "end\n\n";
			fout << "function " << className << ":OnDestroy()\n";
			fout << "    -- Called when the entity is destroyed\n";
			fout << "end\n";
			fout.close();

			std::filesystem::path relativePath = relative(scriptPath, m_BaseDirectory);
			AssetManager::ImportAsset(relativePath);

			CB_CORE_INFO("Created Lua script: {0}", scriptPath.string());
			m_EntriesDirty = true;

			m_IsRenaming = true;
			m_RenamingPath = scriptPath;
			strncpy_s(m_RenameBuffer, name.c_str(), sizeof(m_RenameBuffer) - 1);
		}
	}

	void ContentBrowserPanel::CreateGameManagerScript(const std::string& name)
	{
		std::filesystem::path scriptPath = m_CurrentDirectory / (name + ".lua");

		int counter = 1;
		while (exists(scriptPath)) {
			scriptPath = m_CurrentDirectory / (name + " " + std::to_string(counter) + ".lua");
			counter++;
		}

		std::string className = SanitizeLuaClassName(name);

		std::ofstream fout(scriptPath);
		if (fout.is_open()) {
			fout << "-- " << name << ".lua\n";
			fout << "-- Inherits from GameManager base class\n\n";
			fout << className << " = GameManager:Extend()\n\n";
			fout << className << ".__fields = {\n";
			fout << "    -- MaxScore = Int(100),\n";
			fout << "    -- GameTime = Float(120.0, 0, 600),\n";
			fout << "}\n\n";
			fout << "function " << className << ":OnCreate()\n";
			fout << "    -- Called when the game manager is created\n";
			fout << "end\n\n";
			fout << "function " << className << ":OnUpdate(dt)\n";
			fout << "    -- Called every frame\n";
			fout << "end\n\n";
			fout << "function " << className << ":OnDestroy()\n";
			fout << "    -- Called when the game manager is destroyed\n";
			fout << "end\n";
			fout.close();

			std::filesystem::path relativePath = relative(scriptPath, m_BaseDirectory);
			AssetManager::ImportAsset(relativePath);

			CB_CORE_INFO("Created GameManager script: {0}", scriptPath.string());
			m_EntriesDirty = true;

			m_IsRenaming = true;
			m_RenamingPath = scriptPath;
			strncpy_s(m_RenameBuffer, name.c_str(), sizeof(m_RenameBuffer) - 1);
		}
	}

	void ContentBrowserPanel::ImportExternalFiles(const std::vector<std::string>& paths)
	{
		for (const auto& sourcePath : paths) {
			std::filesystem::path source(sourcePath);

			if (!exists(source)) {
				CB_CORE_WARN("File does not exist: {0}", sourcePath);
				continue;
			}

			// Raw mesh files: open import preview directly from external path
			// They are not copied into the assets folder.
			if (!is_directory(source)) {
				String ext = source.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
				if (IsRawMeshExtension(ext)) {
					EditorLayer::RequestImportPreview(source, m_CurrentDirectory);
					CB_CORE_INFO("Opened import preview for: {0}", sourcePath);
					continue;
				}
			}

			std::filesystem::path destPath = m_CurrentDirectory / source.filename();

			// Handle duplicate names
			if (exists(destPath)) {
				String stem = source.stem().string();
				String ext = source.extension().string();
				int counter = 1;
				while (exists(destPath)) {
					destPath = m_CurrentDirectory / (stem + "_" + std::to_string(counter) + ext);
					counter++;
				}
			}

			std::error_code ec;
			if (is_directory(source)) {
				// Copy directory recursively
				std::filesystem::copy(source, destPath, std::filesystem::copy_options::recursive, ec);
			}
			else {
				// Copy file
				copy_file(source, destPath, ec);
			}

			if (ec) {
				CB_CORE_ERROR("Failed to copy {0}: {1}", sourcePath, ec.message());
				continue;
			}

			// Import the new asset
			if (!is_directory(destPath)) {
				std::filesystem::path relativePath = relative(destPath, m_BaseDirectory);
				AssetManager::ImportAsset(relativePath);
			}

			CB_CORE_INFO("Imported: {0} -> {1}", sourcePath, destPath.string());
		}

		m_EntriesDirty = true;
		Refresh();
	}
}