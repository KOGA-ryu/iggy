#include "scene/level/LevelCollisionCacheState.hpp"

namespace iggy {

LevelCollisionCacheBuildResult LevelCollisionCacheBuilder::build(const LevelTileMap &map) const
{
	LevelCollisionCacheBuildResult result;
	result.collisionWorld = LevelCollisionWorldBuilder {}.build(map);
	if (!result.collisionWorld.built)
		return result;

	result.built = true;
	result.state.world = result.collisionWorld.world;
	return result;
}

} // namespace iggy
