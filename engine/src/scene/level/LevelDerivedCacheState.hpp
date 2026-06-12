#pragma once

#include <vector>

#include "scene/level/LevelCollisionCacheState.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"

namespace iggy {

struct LevelDerivedCacheState {
	bool hasRenderCache = false;
	LevelRenderCacheState render;
	bool hasCollisionCache = false;
	LevelCollisionCacheState collision;
};

struct LevelDerivedCacheBuildConfig {
	bool buildRenderCache = false;
	LevelTileRenderChunkCacheConfig renderCacheConfig;
	bool buildCollisionCache = false;
};

struct LevelDerivedCacheBuildResult {
	bool built = false;
	LevelDerivedCacheState state;
	LevelRenderCacheBuildResult render;
	LevelCollisionCacheBuildResult collision;
};

class LevelDerivedCacheBuilder {
public:
	[[nodiscard]] LevelDerivedCacheBuildResult build(
		const LevelRuntimeState &level,
		const LevelDerivedCacheBuildConfig &config) const;
};

struct LevelDerivedCacheUpdateResult {
	bool updated = false;
	LevelDerivedCacheState state;
	LevelRenderCacheUpdateResult render;
	LevelCollisionCacheUpdateResult collision;
	std::vector<TileCoord> changedTiles;
};

class LevelDerivedCacheUpdater {
public:
	[[nodiscard]] LevelDerivedCacheUpdateResult update(
		const LevelDerivedCacheState &current,
		const LevelRuntimeState &level,
		const std::vector<TileCoord> &changedTiles) const;
};

} // namespace iggy
