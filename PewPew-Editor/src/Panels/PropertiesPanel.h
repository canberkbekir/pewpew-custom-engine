#pragma once

#include "Panel.h"
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

	private:
		// Add your own drawing methods here
		void DrawAssetProperties(UUID assetUUID);
		void DrawEntityProperties(UUID entityUUID);
	};
}
