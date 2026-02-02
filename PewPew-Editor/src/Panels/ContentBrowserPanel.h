#pragma once

#include "Panel.h"
#include "PewPew/Renderer/Resources/Texture.h"

#include <filesystem>
#include <unordered_map>
#include <set>

namespace PewPew
{
	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender() override;

		// Get selected items (for external use)
		const std::set<std::filesystem::path>& GetSelectedItems() const { return m_SelectedItems; }

	private:
		Ref<Texture2D> GetIconForEntry(const std::filesystem::directory_entry& entry);
		void RenderDirectoryTree(const std::filesystem::path& directory);
		void RenderContentItem(const std::filesystem::directory_entry& entry);
		bool PassesFilter(const String& filename) const;

	private:
		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;

		float m_TreeWidth = 200.0f;

		// Icons
		Ref<Texture2D> m_FolderIcon;
		Ref<Texture2D> m_FileIcon;
		std::unordered_map<std::string, Ref<Texture2D>> m_FileTypeIcons;
		std::unordered_map<std::string, Ref<Texture2D>> m_ThumbnailCache;

		float m_IconSize = 64.0f;
		float m_Padding = 16.0f;

		// Search
		char m_SearchBuffer[256] = "";

		// Selection
		std::set<std::filesystem::path> m_SelectedItems;

		// Drag & Drop
		std::filesystem::path m_DraggedItem;
		bool m_IsDragging = false;

		bool IsImageFile(const String& extension) const;
	};
}
