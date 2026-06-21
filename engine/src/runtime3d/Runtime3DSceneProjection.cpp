#include "runtime3d/Runtime3DSceneProjection.hpp"

namespace iggy::runtime3d {

Runtime3DSceneProjection ProjectRuntime3DScene(const Runtime3DWorldState &world)
{
	Runtime3DSceneProjection projection;
	projection.items.reserve(world.entities.size());
	for (const Runtime3DEntityState &entity : world.entities)
		projection.items.push_back({ entity.id, entity.kind, entity.transform, entity.assetRef });
	return projection;
}

} // namespace iggy::runtime3d
