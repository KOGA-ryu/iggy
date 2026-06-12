#pragma once

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

} // namespace iggy
