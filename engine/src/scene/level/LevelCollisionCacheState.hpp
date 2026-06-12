#pragma once

#include <vector>

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

struct LevelCollisionCacheUpdateResult {
	bool updated = false;
	LevelCollisionCacheState state;
	LevelCollisionCacheBuildResult rebuild;
	std::vector<TileCoord> changedTiles;
};

class LevelCollisionCacheUpdater {
public:
	[[nodiscard]] LevelCollisionCacheUpdateResult update(
		const LevelCollisionCacheState &current,
		const LevelTileMap &map,
		const std::vector<TileCoord> &changedTiles) const;
};

} // namespace iggy
