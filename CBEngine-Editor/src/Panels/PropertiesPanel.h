#pragma once

#include "Panel.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Selection/Selectable.h"

namespace CB
{
	class PropertiesPanel : public Panel
	{
	public:
		PropertiesPanel()
			: Panel("Properties", true) { m_LockIcon = Texture2D::Create("resources/icons/lock.png"); }

		void OnImGuiRender() override;

		// Lock the panel to current selection
		void Lock()
		{
			m_Locked = true;
			m_LockedSelection = GetCurrentSelection();
		}

		void Unlock() { m_Locked = false; }
		bool IsLocked() const { return m_Locked; }
	private:
		void DrawAssetProperties(UUID assetUUID);
		Selectable GetCurrentSelection() const;

		bool m_Locked = false;
		Selectable m_LockedSelection;

		//Icons
		Ref<Texture2D> m_LockIcon;
	};
}