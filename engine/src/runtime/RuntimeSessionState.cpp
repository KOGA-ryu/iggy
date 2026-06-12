#include "runtime/RuntimeSessionState.hpp"

#include <utility>

namespace iggy::runtime {

namespace {

void ApplyPlayerConfig(RuntimeSessionState &state, const RuntimeSessionBuildConfig &config)
{
	if (!config.hasPlayer)
		return;

	state.player = config.player;
	state.hasPlayer = true;
}

void MirrorLegacyRenderCache(RuntimeSessionState &state, const LevelRenderCacheState &renderCache)
{
	state.renderCache = renderCache;
	state.hasRenderCache = true;
	state.derivedCaches.hasRenderCache = true;
	state.derivedCaches.render = renderCache;
}

} // namespace

RuntimeSessionBuildResult RuntimeSessionBuilder::build(LevelRuntimeState level, const RuntimeSessionBuildConfig &config) const
{
	RuntimeSessionBuildResult result;
	if (config.buildDerivedCaches) {
		result.derivedCaches = LevelDerivedCacheBuilder {}.build(level, config.derivedCacheConfig);
		result.renderCache = result.derivedCaches.render;
		if (!result.derivedCaches.built)
			return result;

		result.built = true;
		result.state.level = std::move(level);
		result.state.tickIndex = 0;
		result.state.derivedCaches = result.derivedCaches.state;
		if (result.state.derivedCaches.hasRenderCache) {
			result.state.renderCache = result.state.derivedCaches.render;
			result.state.hasRenderCache = true;
		}
		ApplyPlayerConfig(result.state, config);
		return result;
	}

	if (!config.buildRenderCache) {
		result.built = true;
		result.state.level = std::move(level);
		result.state.tickIndex = 0;
		result.state.hasRenderCache = false;
		ApplyPlayerConfig(result.state, config);
		return result;
	}

	result.renderCache = LevelRenderCacheBuilder {}.build(level.map, config.renderCacheConfig);
	if (!result.renderCache.built)
		return result;

	result.built = true;
	result.state.level = std::move(level);
	result.state.tickIndex = 0;
	MirrorLegacyRenderCache(result.state, result.renderCache.state);
	ApplyPlayerConfig(result.state, config);
	return result;
}

} // namespace iggy::runtime
