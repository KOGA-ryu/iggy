#pragma once

#include "runtime3d/Runtime3DEntityId.hpp"
#include "runtime3d/Runtime3DWorldState.hpp"

namespace iggy::runtime3d {

struct Runtime3DTargetQueryResult {
	bool found = false;
	Runtime3DEntityId entityId;
};

[[nodiscard]] Runtime3DTargetQueryResult FindFirstRuntime3DTarget(const Runtime3DWorldState &world);

} // namespace iggy::runtime3d
