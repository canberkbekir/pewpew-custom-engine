#pragma once

#include "SubstanceID.h"
#include "VoxelSubstance.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

namespace CB
{
	struct SubstanceDefinition
	{
		SubstanceID ID;
		Vector3 DisplayColor = {0.5f, 0.5f, 0.5f};
		VMatMaterialProperties PBRDefaults;
		VoxelSubstanceProperties Properties;
	};
}
