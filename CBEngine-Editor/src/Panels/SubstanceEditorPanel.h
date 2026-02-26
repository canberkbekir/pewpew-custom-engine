#pragma once
#include "Panel.h"
#include "CBEngine/Utils/VoxelMaterialType.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstance.h"

namespace CB
{
	class SubstanceEditorPanel : public Panel
	{
	public:
		SubstanceEditorPanel()
			: Panel("Substance Editor", false)
		{
		} // hidden by default

		void OnImGuiRender() override;
	private:
		void DrawSubstance(VoxelMaterialType type);
		void DrawDamageTints(VoxelSubstanceProperties& props);

		int m_SelectedSubstance = 0;
	};
}