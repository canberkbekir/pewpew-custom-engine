#pragma once

#include "Panel.h"
#include "PewPew/Renderer/Resources/Texture.h"

#include <filesystem>
#include <unordered_map>

namespace PewPew
{
	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender() override;

	private:
		Ref<Texture2D> GetIconForEntry(const std::filesystem::directory_entry& entry);

	private:
		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;

		// Icons
		Ref<Texture2D> m_FolderIcon;
		Ref<Texture2D> m_FileIcon;
		std::unordered_map<std::string, Ref<Texture2D>> m_FileTypeIcons;
		std::unordered_map<std::string, Ref<Texture2D>> m_ThumbnailCache;

		float m_IconSize = 64.0f;
		float m_Padding = 16.0f;

		bool IsImageFile(const std::string& extension) const;
	};
}
