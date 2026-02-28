#pragma once

#include "SubstancePropertyMeta.h"
#include <vector>

namespace CB
{
	// Returns the full schema of all VoxelSubstanceProperties fields.
	// Built once on first call, cached statically.
	const std::vector<SubstancePropertyMeta>& GetSubstancePropertySchema();
}
