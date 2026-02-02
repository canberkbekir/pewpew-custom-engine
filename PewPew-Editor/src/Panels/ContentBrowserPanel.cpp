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

		// Back button
		if (m_CurrentDirectory != m_BaseDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
			ImGui::SameLine();
		}

		// Show current path
		ImGui::TextDisabled("%s", m_CurrentDirectory.string().c_str());
		ImGui::Separator();

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
				const auto& path = entry.path();
				String filename = path.filename().string();

				ImGui::PushID(filename.c_str());

				// Get appropriate icon
				Ref<Texture2D> icon = GetIconForEntry(entry);

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::ImageButton(
					reinterpret_cast<void*>(static_cast<uint64_t>(icon->GetRendererID())),
					{ m_IconSize, m_IconSize },
					{ 0, 1 }, { 1, 0 }
				);
				ImGui::PopStyleColor();

				// Handle double-click for directories
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (entry.is_directory())
					{
						m_CurrentDirectory = path;
					}
				}

				// Filename text (centered, truncated if needed)
				ImGui::TextWrapped("%s", filename.c_str());

				ImGui::NextColumn();
				ImGui::PopID();
			}
		}
		else
		{
			ImGui::TextDisabled("Directory not found: %s", m_CurrentDirectory.string().c_str());
		}

		ImGui::Columns(1);
		ImGui::End();
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
