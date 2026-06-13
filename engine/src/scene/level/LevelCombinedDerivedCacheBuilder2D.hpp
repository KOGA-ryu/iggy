#pragma once

#include "scene/level/LevelCollisionCacheState.hpp"
#include "scene/level/LevelCollisionSource2D.hpp"
#include "scene/level/LevelCollisionSourceWorldBuilder2D.hpp"
#include "scene/level/LevelCollisionWorldMerge2D.hpp"
#include "scene/level/LevelDerivedCacheState.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"

namespace iggy {

struct LevelCombinedDerivedCacheBuildConfig2D {
	bool buildRenderCache = false;
	LevelTileRenderChunkCacheConfig renderCacheConfig;
	bool buildCollisionCache = false;
	LevelCollisionSourceWorldBuilder2DConfig sourceWorldConfig;
};

struct LevelCombinedDerivedCacheBuildResult2D {
	bool built = false;
	LevelDerivedCacheState state;
	LevelRenderCacheBuildResult render;
	LevelCollisionCacheBuildResult tileCollision;
	LevelCollisionSourceWorldBuilder2DResult sourceCollision;
	LevelCollisionWorldMerge2DResult merge;
};

class LevelCombinedDerivedCacheBuilder2D {
public:
	[[nodiscard]] LevelCombinedDerivedCacheBuildResult2D build(
		const LevelRuntimeState &level,
		const LevelCollisionSource2D &source,
		const LevelCombinedDerivedCacheBuildConfig2D &config) const;
};

} // namespace iggy
