#include "runtime3d/Runtime3DTargetQuery.hpp"

namespace iggy::runtime3d {

Runtime3DTargetQueryResult FindFirstRuntime3DTarget(const Runtime3DWorldState &world)
{
	for (const Runtime3DEntityState &entity : world.entities) {
		if (entity.id.valid() && IsRuntime3DTargetableKind(entity.kind))
			return { true, entity.id };
	}

	return {};
}

} // namespace iggy::runtime3d
