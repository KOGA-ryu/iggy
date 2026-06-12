#pragma once

#include <vector>

#include "scene/level/LevelDerivedCacheState.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "scene/level/LevelTileMutation.hpp"

namespace iggy {

enum class LevelMutationCacheUpdateStatus {
	NoMutation,
	Updated,
	MutationOnly,
	CacheUpdateFailed,
};

struct LevelMutationCacheUpdateResult {
	LevelMutationCacheUpdateStatus status = LevelMutationCacheUpdateStatus::NoMutation;
	LevelRuntimeState level;
	LevelDerivedCacheState derivedCaches;
	LevelTileMutationResult mutation;
	LevelDerivedCacheUpdateResult cacheUpdate;
};

class LevelMutationCacheUpdateStep {
public:
	[[nodiscard]] LevelMutationCacheUpdateResult apply(
		const LevelRuntimeState &level,
		const LevelDerivedCacheState &currentCaches,
		const std::vector<LevelTileEdit> &edits) const;
};

} // namespace iggy
