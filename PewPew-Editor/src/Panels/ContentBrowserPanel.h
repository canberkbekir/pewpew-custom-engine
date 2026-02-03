#pragma once

#include "Panel.h"
#include "PewPew/Renderer/Resources/Texture.h"
#include "PewPew/Core/UUID.h"
#include "PewPew/Asset/Asset.h"

#include <filesystem>
#include <unordered_map>
#include <set>
#include <vector>

namespace PewPew
{
	enum class SortMode
	{
		Name,
		Type,
		DateModified
	};

	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender() override;
		void OnEvent(Event& e) override;

	private:
		// Rendering
		Ref<Texture2D> GetIconForEntry(const std::filesystem::directory_entry& entry);
		void RenderDirectoryTree(const std::filesystem::path& directory);
		void RenderContentItem(const std::filesystem::directory_entry& entry);
		void RenderToolbar();
		void RenderBreadcrumbs();
		void RenderContextMenu();

		// Filtering & Sorting
		bool PassesFilter(const std::filesystem::directory_entry& entry) const;
		bool PassesTypeFilter(const std::filesystem::directory_entry& entry) const;
		std::vector<std::filesystem::directory_entry> GetSortedEntries() const;

		// Selection
		void SelectAsset(const std::filesystem::path& path, bool addToSelection = false);
		void SelectRange(const std::filesystem::path& from, const std::filesystem::path& to);
		void SelectAll();
		bool IsAssetSelected(const std::filesystem::path& path) const;

		// File Operations
		void Refresh();
		void DeleteSelected();
		void DuplicateSelected();
		void RenameAsset(const std::filesystem::path& path, const std::string& newName);
		void MoveAsset(const std::filesystem::path& source, const std::filesystem::path& destFolder);
		void CreateFolder(const std::string& name);
		void CreateMaterial(const std::string& name);

		// Context Menu Actions
		void ShowInExplorer(const std::filesystem::path& path);
		void CopyPathToClipboard(const std::filesystem::path& path);
		void ReimportAsset(const std::filesystem::path& path);

		// Navigation
		void NavigateTo(const std::filesystem::path& directory);
		void NavigateBack();
		void NavigateForward();

		// Keyboard handling
		void HandleKeyboardInput();

		// External file import
		void ImportExternalFiles(const std::vector<std::string>& paths);
		bool OnFileDrop(class FileDropEvent& e);

		// Helpers
		bool IsImageFile(const String& extension) const;
		int GetEntryIndex(const std::filesystem::path& path) const;

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

		// Sort
		SortMode m_SortMode = SortMode::Name;
		bool m_SortAscending = true;

		// Type filter
		bool m_ShowAllTypes = true;
		bool m_ShowTextures = true;
		bool m_ShowMeshes = true;
		bool m_ShowShaders = true;
		bool m_ShowMaterials = true;
		bool m_ShowScenes = true;
		bool m_ShowOther = true;

		// Selection
		std::set<std::filesystem::path> m_SelectedPaths;
		std::filesystem::path m_LastSelectedPath;  // For shift-click range select

		// Navigation history
		std::vector<std::filesystem::path> m_BackHistory;
		std::vector<std::filesystem::path> m_ForwardHistory;

		// Drag & Drop
		std::filesystem::path m_DraggedItem;
		bool m_IsDragging = false;

		// Rename state
		bool m_IsRenaming = false;
		std::filesystem::path m_RenamingPath;
		char m_RenameBuffer[256] = "";

		// Cached entries for keyboard navigation
		mutable std::vector<std::filesystem::directory_entry> m_CachedEntries;
		mutable bool m_EntriesDirty = true;

		// Track if panel is hovered for external file drops
		bool m_IsWindowHovered = false;
	};
}
