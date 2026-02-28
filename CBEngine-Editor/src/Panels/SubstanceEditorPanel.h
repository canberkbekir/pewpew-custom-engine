#pragma once
#include "Panel.h"
#include "CBEngine/Voxel/Substance/SubstanceID.h"

#include <string>

namespace CB
{
	class SubstanceEditorPanel : public Panel
	{
	public:
		SubstanceEditorPanel()
			: Panel("Substance Editor", false)
		{
		}

		void OnImGuiRender() override;
	private:
		void DrawSubstanceList();
		void DrawSubstanceInspector();

		SubstanceID m_SelectedID;
		char m_SearchFilter[128] = {};
		char m_RenameBuffer[128] = {};
		bool m_ShowRenamePopup = false;
	};
}
