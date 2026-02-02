#pragma once

#include "Selectable.h"

#include <vector>
#include <functional>

namespace PewPew
{
	using SelectionChangedCallback = std::function<void(const std::vector<Selectable>&)>;

	class Selection
	{
	public:
		// Single selection (clears previous selection)
		static void Select(const Selectable& item);

		// Multi-selection
		static void AddToSelection(const Selectable& item);
		static void RemoveFromSelection(const Selectable& item);
		static void SetSelection(const std::vector<Selectable>& items);

		// Clear selection
		static void Clear();

		// Query
		static const std::vector<Selectable>& GetSelection() { return s_Selection; }
		static Selectable GetPrimarySelection();
		static bool IsSelected(const Selectable& item);
		static bool HasSelection() { return !s_Selection.empty(); }
		static size_t GetSelectionCount() { return s_Selection.size(); }

		// Type-specific queries
		static bool HasAssetSelected();
		static bool HasEntitySelected();
		static std::vector<UUID> GetSelectedAssets();
		static std::vector<UUID> GetSelectedEntities();

		// Callbacks
		static void AddSelectionChangedCallback(SelectionChangedCallback callback);
		static void RemoveAllCallbacks();

	private:
		static void NotifySelectionChanged();

	private:
		static std::vector<Selectable> s_Selection;
		static std::vector<SelectionChangedCallback> s_Callbacks;
	};
}
