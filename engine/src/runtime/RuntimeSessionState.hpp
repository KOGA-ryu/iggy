#pragma once

#include <cstddef>

#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRuntimeState.hpp"

namespace iggy::runtime {

struct RuntimeSessionState {
	LevelRuntimeState level;
	LevelRenderCacheState renderCache;
	std::size_t tickIndex = 0;
	bool hasRenderCache = false;
};

struct RuntimeSessionBuildConfig {
	LevelTileRenderChunkCacheConfig renderCacheConfig;
	bool buildRenderCache = true;
};

struct RuntimeSessionBuildResult {
	bool built = false;
	RuntimeSessionState state;
	LevelRenderCacheBuildResult renderCache;
};

class RuntimeSessionBuilder {
public:
	[[nodiscard]] RuntimeSessionBuildResult build(LevelRuntimeState level, const RuntimeSessionBuildConfig &config) const;
};

} // namespace iggy::runtime
