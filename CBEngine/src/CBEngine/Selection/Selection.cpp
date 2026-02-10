#include "cbpch.h"
#include "Selection.h"

#include <algorithm>

namespace CB
{
    std::vector<Selectable> Selection::s_Selection;
    std::vector<SelectionChangedCallback> Selection::s_Callbacks;

    void Selection::Select(const Selectable& item)
    {
        s_Selection.clear();

        if (item.IsValid())
        {
            s_Selection.push_back(item);
        }

        NotifySelectionChanged();
    }

    void Selection::AddToSelection(const Selectable& item)
    {
        if (!item.IsValid())
            return;

        // Don't add duplicates
        if (!IsSelected(item))
        {
            s_Selection.push_back(item);
            NotifySelectionChanged();
        }
    }

    void Selection::RemoveFromSelection(const Selectable& item)
    {
        auto it = std::find(s_Selection.begin(), s_Selection.end(), item);
        if (it != s_Selection.end())
        {
            s_Selection.erase(it);
            NotifySelectionChanged();
        }
    }

    void Selection::SetSelection(const std::vector<Selectable>& items)
    {
        s_Selection = items;
        NotifySelectionChanged();
    }

    void Selection::Clear()
    {
        if (!s_Selection.empty())
        {
            s_Selection.clear();
            NotifySelectionChanged();
        }
    }

    Selectable Selection::GetPrimarySelection()
    {
        if (s_Selection.empty())
            return Selectable::Invalid();
        return s_Selection[0];
    }

    bool Selection::IsSelected(const Selectable& item)
    {
        return std::find(s_Selection.begin(), s_Selection.end(), item) != s_Selection.end();
    }

    bool Selection::HasAssetSelected()
    {
        for (const auto& item : s_Selection)
        {
            if (item.Type == SelectableType::Asset)
                return true;
        }
        return false;
    }

    bool Selection::HasEntitySelected()
    {
        for (const auto& item : s_Selection)
        {
            if (item.Type == SelectableType::Entity)
                return true;
        }
        return false;
    }

    std::vector<UUID> Selection::GetSelectedAssets()
    {
        std::vector<UUID> assets;
        for (const auto& item : s_Selection)
        {
            if (item.Type == SelectableType::Asset)
                assets.push_back(item.ID);
        }
        return assets;
    }

    std::vector<UUID> Selection::GetSelectedEntities()
    {
        std::vector<UUID> entities;
        for (const auto& item : s_Selection)
        {
            if (item.Type == SelectableType::Entity)
                entities.push_back(item.ID);
        }
        return entities;
    }

    void Selection::AddSelectionChangedCallback(SelectionChangedCallback callback)
    {
        s_Callbacks.push_back(callback);
    }

    void Selection::RemoveAllCallbacks()
    {
        s_Callbacks.clear();
    }

    void Selection::NotifySelectionChanged()
    {
        for (const auto& callback : s_Callbacks)
        {
            callback(s_Selection);
        }
    }
}
