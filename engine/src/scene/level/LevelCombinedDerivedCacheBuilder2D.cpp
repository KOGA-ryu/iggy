#include "scene/level/LevelCombinedDerivedCacheBuilder2D.hpp"

namespace iggy {

LevelCombinedDerivedCacheBuildResult2D LevelCombinedDerivedCacheBuilder2D::build(
	const LevelRuntimeState &level,
	const LevelCollisionSource2D &source,
	const LevelCombinedDerivedCacheBuildConfig2D &config) const
{
	LevelCombinedDerivedCacheBuildResult2D result;

	if (config.buildRenderCache)
		result.render = LevelRenderCacheBuilder {}.build(level.map, config.renderCacheConfig);

	if (config.buildCollisionCache) {
		result.tileCollision = LevelCollisionCacheBuilder {}.build(level.map);
		if (result.tileCollision.built)
			result.sourceCollision = LevelCollisionSourceWorldBuilder2D {}.build(source, config.sourceWorldConfig);
		if (result.tileCollision.built && result.sourceCollision.built) {
			result.merge = LevelCollisionWorldMerge2D {}.merge(
				result.tileCollision.state.world,
				result.sourceCollision.world);
		}
	}

	if (config.buildRenderCache && !result.render.built)
		return result;

	if (config.buildCollisionCache && !result.tileCollision.built)
		return result;

	if (config.buildCollisionCache && !result.sourceCollision.built)
		return result;

	if (config.buildCollisionCache && !result.merge.built)
		return result;

	result.built = true;
	if (config.buildRenderCache) {
		result.state.hasRenderCache = true;
		result.state.render = result.render.state;
	}
	if (config.buildCollisionCache) {
		result.state.hasCollisionCache = true;
		result.state.collision.world = result.merge.world;
	}

	return result;
}

} // namespace iggy
