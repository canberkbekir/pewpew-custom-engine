#include "ContentBrowserPanel.h"

#include "imgui.h"

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
			// Toolbar: Back button + Search
			if (m_CurrentDirectory != m_BaseDirectory)
			{
				if (ImGui::Button("<-"))
				{
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
				}
				ImGui::SameLine();
			}

			// Search bar
			ImGui::PushItemWidth(200);
			ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer));
			ImGui::PopItemWidth();

			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				m_SearchBuffer[0] = '\0';
			}

			// Show current path
			ImGui::TextDisabled("%s", m_CurrentDirectory.string().c_str());

			// Selection info
			if (!m_SelectedItems.empty())
			{
				ImGui::SameLine();
				ImGui::Text("| %zu selected", m_SelectedItems.size());
			}

			ImGui::Separator();

			// Click on empty space to deselect all
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
			{
				if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
				{
					m_SelectedItems.clear();
				}
			}

			// Calculate how many icons fit per row
			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = static_cast<int>(panelWidth / (m_IconSize + m_Padding));
			if (columnCount < 1)
				columnCount = 1;

			ImGui::Columns(columnCount, nullptr, false);

			// Directory contents
			if (std::filesystem::exists(m_CurrentDirectory))
			{
				for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
				{
					String filename = entry.path().filename().string();

					// Filter by search
					if (!PassesFilter(filename))
						continue;

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

	void ContentBrowserPanel::RenderContentItem(const std::filesystem::directory_entry& entry)
	{
		const auto& path = entry.path();
		String filename = path.filename().string();
		bool isSelected = m_SelectedItems.count(path) > 0;

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
			if (ImGui::GetIO().KeyCtrl)
			{
				// Ctrl+Click: Toggle selection
				if (isSelected)
					m_SelectedItems.erase(path);
				else
					m_SelectedItems.insert(path);
			}
			else
			{
				// Normal click: Select only this item
				m_SelectedItems.clear();
				m_SelectedItems.insert(path);
			}
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

			if (m_SelectedItems.size() > 1)
			{
				ImGui::Text("(+%zu more)", m_SelectedItems.size() - 1);
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
				m_SelectedItems.clear();
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

	bool ContentBrowserPanel::PassesFilter(const String& filename) const
	{
		if (m_SearchBuffer[0] == '\0')
			return true;

		// Case-insensitive search
		String lowerFilename = filename;
		String lowerSearch = m_SearchBuffer;
		std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
		std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

		return lowerFilename.find(lowerSearch) != String::npos;
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
}
