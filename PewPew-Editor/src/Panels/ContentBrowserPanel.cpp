#include "ContentBrowserPanel.h"

#include "imgui.h"
#include "PewPew/Asset/AssetManager.h"
#include "PewPew/Selection/Selection.h"
#include "PewPew/Core/Log.h"

#include <algorithm>

namespace PewPew
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
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::Begin(m_Name.c_str(), &m_Visible);

		// Left panel - Directory tree
		ImGui::BeginChild("DirectoryTree", ImVec2(m_TreeWidth, 0), true);
		{
			if (std::filesystem::exists(m_BaseDirectory))
			{
				RenderDirectoryTree(m_BaseDirectory);
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Splitter
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::Button("##Splitter", ImVec2(4.0f, -1));
		ImGui::PopStyleColor(3);

		if (ImGui::IsItemActive())
		{
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

			ImGui::Separator();

			// Click on empty space to deselect all
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
			{
				if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
				{
					m_SelectedPaths.clear();
					Selection::Clear();
				}
			}

			// Calculate how many icons fit per row
			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = static_cast<int>(panelWidth / (m_IconSize + m_Padding));
			if (columnCount < 1)
				columnCount = 1;

			ImGui::Columns(columnCount, nullptr, false);

			// Directory contents (sorted and filtered)
			if (std::filesystem::exists(m_CurrentDirectory))
			{
				auto entries = GetSortedEntries();
				for (const auto& entry : entries)
				{
					RenderContentItem(entry);
				}
			}
			else
			{
				ImGui::TextDisabled("Directory not found: %s", m_CurrentDirectory.string().c_str());
			}

			ImGui::Columns(1);
		}
		ImGui::EndChild();

		ImGui::End();
	}

	void ContentBrowserPanel::RenderToolbar()
	{
		// Back button
		if (m_CurrentDirectory != m_BaseDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
			ImGui::SameLine();
		}

		// Refresh button
		if (ImGui::Button("Refresh"))
		{
			Refresh();
		}
		ImGui::SameLine();

		// Search bar
		ImGui::PushItemWidth(150);
		ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer));
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("X##ClearSearch"))
		{
			m_SearchBuffer[0] = '\0';
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Sort dropdown
		ImGui::Text("Sort:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		const char* sortModes[] = { "Name", "Type", "Date Modified" };
		int currentSort = static_cast<int>(m_SortMode);
		if (ImGui::Combo("##SortMode", &currentSort, sortModes, IM_ARRAYSIZE(sortModes)))
		{
			m_SortMode = static_cast<SortMode>(currentSort);
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button(m_SortAscending ? "Asc" : "Desc"))
		{
			m_SortAscending = !m_SortAscending;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Toggle sort order");

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Type filter dropdown
		ImGui::Text("Filter:");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##TypeFilter", m_ShowAllTypes ? "All" : "Custom"))
		{
			if (ImGui::Checkbox("All Types", &m_ShowAllTypes))
			{
				if (m_ShowAllTypes)
				{
					m_ShowTextures = true;
					m_ShowMeshes = true;
					m_ShowShaders = true;
					m_ShowMaterials = true;
					m_ShowScenes = true;
					m_ShowOther = true;
				}
			}
			ImGui::Separator();

			bool anyChanged = false;
			anyChanged |= ImGui::Checkbox("Textures", &m_ShowTextures);
			anyChanged |= ImGui::Checkbox("Meshes", &m_ShowMeshes);
			anyChanged |= ImGui::Checkbox("Shaders", &m_ShowShaders);
			anyChanged |= ImGui::Checkbox("Materials", &m_ShowMaterials);
			anyChanged |= ImGui::Checkbox("Scenes", &m_ShowScenes);
			anyChanged |= ImGui::Checkbox("Other", &m_ShowOther);

			if (anyChanged)
			{
				m_ShowAllTypes = m_ShowTextures && m_ShowMeshes && m_ShowShaders &&
				                 m_ShowMaterials && m_ShowScenes && m_ShowOther;
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Thumbnail size slider
		ImGui::Text("Size:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::SliderFloat("##IconSize", &m_IconSize, 32.0f, 128.0f, "%.0f");
		ImGui::PopItemWidth();

		// Second row: path and selection info
		ImGui::TextDisabled("%s", m_CurrentDirectory.string().c_str());

		if (!m_SelectedPaths.empty())
		{
			ImGui::SameLine();
			ImGui::Text("| %zu selected", m_SelectedPaths.size());
		}
	}

	std::vector<std::filesystem::directory_entry> ContentBrowserPanel::GetSortedEntries() const
	{
		std::vector<std::filesystem::directory_entry> entries;

		for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
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
			[this](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
			{
				// Directories always come first
				if (a.is_directory() != b.is_directory())
					return a.is_directory();

				int cmp = 0;
				switch (m_SortMode)
				{
					case SortMode::Name:
					{
						String nameA = a.path().filename().string();
						String nameB = b.path().filename().string();
						std::transform(nameA.begin(), nameA.end(), nameA.begin(), ::tolower);
						std::transform(nameB.begin(), nameB.end(), nameB.begin(), ::tolower);
						cmp = nameA.compare(nameB);
						break;
					}
					case SortMode::Type:
					{
						String extA = a.path().extension().string();
						String extB = b.path().extension().string();
						std::transform(extA.begin(), extA.end(), extA.begin(), ::tolower);
						std::transform(extB.begin(), extB.end(), extB.begin(), ::tolower);
						cmp = extA.compare(extB);
						// If same extension, sort by name
						if (cmp == 0)
						{
							String nameA = a.path().filename().string();
							String nameB = b.path().filename().string();
							std::transform(nameA.begin(), nameA.end(), nameA.begin(), ::tolower);
							std::transform(nameB.begin(), nameB.end(), nameB.begin(), ::tolower);
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

		// Case-insensitive search
		String lowerFilename = filename;
		String lowerSearch = m_SearchBuffer;
		std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
		std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

		return lowerFilename.find(lowerSearch) != String::npos;
	}

	bool ContentBrowserPanel::PassesTypeFilter(const std::filesystem::directory_entry& entry) const
	{
		// Directories always pass
		if (entry.is_directory())
			return true;

		if (m_ShowAllTypes)
			return true;

		String ext = entry.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		// Textures
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".hdr")
			return m_ShowTextures;

		// Meshes
		if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
			return m_ShowMeshes;

		// Shaders
		if (ext == ".glsl" || ext == ".hlsl" || ext == ".vert" || ext == ".frag" || ext == ".comp")
			return m_ShowShaders;

		// Materials
		if (ext == ".mat" || ext == ".material")
			return m_ShowMaterials;

		// Scenes
		if (ext == ".scene" || ext == ".pewpew")
			return m_ShowScenes;

		// Other/Unknown
		return m_ShowOther;
	}

	void ContentBrowserPanel::Refresh()
	{
		// Clear thumbnail cache to force reload
		m_ThumbnailCache.clear();

		PEW_CORE_INFO("ContentBrowser: Refreshed directory {0}", m_CurrentDirectory.string());
	}

	void ContentBrowserPanel::RenderContentItem(const std::filesystem::directory_entry& entry)
	{
		const auto& path = entry.path();
		String filename = path.filename().string();
		bool isSelected = IsAssetSelected(path);

		ImGui::PushID(filename.c_str());

		// Get appropriate icon
		Ref<Texture2D> icon = GetIconForEntry(entry);

		// Highlight selected items
		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 0.6f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		}

		ImGui::ImageButton(
			reinterpret_cast<void*>(static_cast<uint64_t>(icon->GetRendererID())),
			{ m_IconSize, m_IconSize },
			{ 0, 1 }, { 1, 0 }
		);
		ImGui::PopStyleColor();

		// Selection handling
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			bool addToSelection = ImGui::GetIO().KeyCtrl;
			SelectAsset(path, addToSelection);
		}

		// Drag source
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			m_DraggedItem = path;
			m_IsDragging = true;

			// Set payload - path string
			String pathStr = path.string();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);

			// Drag preview
			ImGui::Image(
				reinterpret_cast<void*>(static_cast<uint64_t>(icon->GetRendererID())),
				ImVec2(32, 32),
				ImVec2(0, 1), ImVec2(1, 0)
			);
			ImGui::SameLine();
			ImGui::Text("%s", filename.c_str());

			if (m_SelectedPaths.size() > 1)
			{
				ImGui::Text("(+%zu more)", m_SelectedPaths.size() - 1);
			}

			ImGui::EndDragDropSource();
		}
		else
		{
			m_IsDragging = false;
		}

		// Drop target (for folders)
		if (entry.is_directory() && ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const char* droppedPath = static_cast<const char*>(payload->Data);
				// TODO: Backend - Move file to this folder
				// std::filesystem::path source(droppedPath);
				// std::filesystem::path dest = path / source.filename();
				// AssetManager::MoveAsset(source, dest);
			}
			ImGui::EndDragDropTarget();
		}

		// Double-click for directories
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (entry.is_directory())
			{
				m_CurrentDirectory = path;
				m_SelectedPaths.clear();
				Selection::Clear();
			}
			else
			{
				// TODO: Backend - Open file with appropriate editor
				// AssetManager::OpenAsset(path);
			}
		}

		// Right-click context menu
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Open"))
			{
				// TODO: Backend - Open asset
			}
			if (ImGui::MenuItem("Rename"))
			{
				// TODO: Backend - Rename asset
			}
			if (ImGui::MenuItem("Delete"))
			{
				// TODO: Backend - Delete asset
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Show in Explorer"))
			{
				// TODO: Backend - Open folder in system file explorer
			}
			ImGui::EndPopup();
		}

		// Filename text
		ImGui::TextWrapped("%s", filename.c_str());

		ImGui::NextColumn();
		ImGui::PopID();
	}

	void ContentBrowserPanel::RenderDirectoryTree(const std::filesystem::path& directory)
	{
		String dirName = directory.filename().string();
		if (dirName.empty())
			dirName = directory.string();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

		// Highlight if this is the current directory
		if (m_CurrentDirectory == directory)
			flags |= ImGuiTreeNodeFlags_Selected;

		// Check if directory has subdirectories
		bool hasSubDirs = false;
		if (std::filesystem::exists(directory))
		{
			for (const auto& entry : std::filesystem::directory_iterator(directory))
			{
				if (entry.is_directory())
				{
					hasSubDirs = true;
					break;
				}
			}
		}

		if (!hasSubDirs)
			flags |= ImGuiTreeNodeFlags_Leaf;

		// Folder icon + tree node
		ImGui::Image(
			reinterpret_cast<void*>(static_cast<uint64_t>(m_FolderIcon->GetRendererID())),
			ImVec2(16, 16),
			ImVec2(0, 1), ImVec2(1, 0)
		);
		ImGui::SameLine();

		bool opened = ImGui::TreeNodeEx(dirName.c_str(), flags);

		// Click to navigate
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			m_CurrentDirectory = directory;
		}

		if (opened)
		{
			if (std::filesystem::exists(directory))
			{
				for (const auto& entry : std::filesystem::directory_iterator(directory))
				{
					if (entry.is_directory())
					{
						RenderDirectoryTree(entry.path());
					}
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
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		// Show actual image as thumbnail
		if (IsImageFile(extension))
		{
			String pathStr = entry.path().string();

			auto it = m_ThumbnailCache.find(pathStr);
			if (it != m_ThumbnailCache.end())
				return it->second;

			// Load and cache thumbnail
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

	void ContentBrowserPanel::SelectAsset(const std::filesystem::path& path, bool addToSelection)
	{
		// Get relative path for AssetManager lookup
		std::filesystem::path relativePath = std::filesystem::relative(path, m_BaseDirectory);
		UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

		PEW_CORE_TRACE("ContentBrowser: SelectAsset - path={0}, relative={1}, uuid={2}",
			path.string(), relativePath.string(), uuid.IsValid() ? std::to_string((uint64_t)uuid) : "INVALID");

		if (addToSelection)
		{
			// Toggle selection
			if (m_SelectedPaths.count(path) > 0)
			{
				m_SelectedPaths.erase(path);
				if (uuid.IsValid())
					Selection::RemoveFromSelection(Selectable::Asset(uuid));
			}
			else
			{
				m_SelectedPaths.insert(path);
				if (uuid.IsValid())
					Selection::AddToSelection(Selectable::Asset(uuid));
			}
		}
		else
		{
			// Single selection
			m_SelectedPaths.clear();
			m_SelectedPaths.insert(path);

			if (uuid.IsValid())
			{
				Selection::Select(Selectable::Asset(uuid));
			}
			else
			{
				// Asset not yet imported - select by path only (local selection)
				Selection::Clear();
			}
		}
	}

	bool ContentBrowserPanel::IsAssetSelected(const std::filesystem::path& path) const
	{
		return m_SelectedPaths.count(path) > 0;
	}
}