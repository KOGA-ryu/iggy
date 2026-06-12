#pragma once

#include "scene/level/LevelCollisionWorldBuilder.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy {

struct LevelCollisionCacheState {
	physics2d::CollisionWorld2D world;
};

struct LevelCollisionCacheBuildResult {
	bool built = false;
	LevelCollisionCacheState state;
	LevelCollisionWorldBuildResult collisionWorld;
};

class LevelCollisionCacheBuilder {
public:
	[[nodiscard]] LevelCollisionCacheBuildResult build(const LevelTileMap &map) const;
};

} // namespace iggy
