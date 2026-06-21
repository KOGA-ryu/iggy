#pragma once

#include <vector>

#include "runtime3d/Runtime3DEntityState.hpp"

namespace iggy::runtime3d {

struct Runtime3DWorldState {
	std::vector<Runtime3DEntityState> entities;

	[[nodiscard]] Runtime3DEntityState *find(Runtime3DEntityId id);
	[[nodiscard]] const Runtime3DEntityState *find(Runtime3DEntityId id) const;
	[[nodiscard]] bool add(Runtime3DEntityState entity);
	[[nodiscard]] bool upsert(Runtime3DEntityState entity);
};

} // namespace iggy::runtime3d
