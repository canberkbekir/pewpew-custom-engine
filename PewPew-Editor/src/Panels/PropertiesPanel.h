#pragma once

#include "Panel.h"
#include "PewPew/Core/Core.h"
#include "PewPew/Core/UUID.h"
#include "PewPew/Selection/Selectable.h"

namespace PewPew
{
	class PropertiesPanel : public Panel
	{
	public:
		PropertiesPanel()
			: Panel("Properties", true)
		{
		}

		void OnImGuiRender() override;

		// Lock the panel to current selection
		void Lock() { m_Locked = true; m_LockedSelection = GetCurrentSelection(); }
		void Unlock() { m_Locked = false; }
		bool IsLocked() const { return m_Locked; }

	private:
		void DrawAssetProperties(UUID assetUUID);
		Selectable GetCurrentSelection() const;

		bool m_Locked = false;
		Selectable m_LockedSelection;
	};
}
