#include "scene/level/LevelDerivedCacheState.hpp"

namespace iggy {

LevelDerivedCacheBuildResult LevelDerivedCacheBuilder::build(
	const LevelRuntimeState &level,
	const LevelDerivedCacheBuildConfig &config) const
{
	LevelDerivedCacheBuildResult result;

	if (config.buildRenderCache)
		result.render = LevelRenderCacheBuilder {}.build(level.map, config.renderCacheConfig);

	if (config.buildCollisionCache)
		result.collision = LevelCollisionCacheBuilder {}.build(level.map);

	if (config.buildRenderCache && !result.render.built)
		return result;

	if (config.buildCollisionCache && !result.collision.built)
		return result;

	result.built = true;
	if (config.buildRenderCache) {
		result.state.hasRenderCache = true;
		result.state.render = result.render.state;
	}
	if (config.buildCollisionCache) {
		result.state.hasCollisionCache = true;
		result.state.collision = result.collision.state;
	}
	return result;
}

} // namespace iggy
