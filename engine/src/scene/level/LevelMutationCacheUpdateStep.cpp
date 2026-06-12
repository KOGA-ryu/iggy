#include "scene/level/LevelMutationCacheUpdateStep.hpp"

namespace iggy {

LevelMutationCacheUpdateResult LevelMutationCacheUpdateStep::apply(
	const LevelRuntimeState &level,
	const LevelDerivedCacheState &currentCaches,
	const std::vector<LevelTileEdit> &edits) const
{
	LevelMutationCacheUpdateResult result;
	result.level = level;
	result.derivedCaches = currentCaches;
	result.mutation = LevelTileMutation {}.apply(level.map, edits);
	result.level.map = result.mutation.map;

	if (result.mutation.changedTiles.empty())
		return result;

	result.cacheUpdate = LevelDerivedCacheUpdater {}.update(currentCaches, result.level, result.mutation.changedTiles);
	if (!result.cacheUpdate.updated) {
		result.status = LevelMutationCacheUpdateStatus::CacheUpdateFailed;
		return result;
	}

	result.derivedCaches = result.cacheUpdate.state;
	result.status = currentCaches.hasRenderCache || currentCaches.hasCollisionCache
		? LevelMutationCacheUpdateStatus::Updated
		: LevelMutationCacheUpdateStatus::MutationOnly;
	return result;
}

} // namespace iggy
