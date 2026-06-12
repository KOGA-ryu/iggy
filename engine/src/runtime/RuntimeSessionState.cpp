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

} // namespace

RuntimeSessionBuildResult RuntimeSessionBuilder::build(LevelRuntimeState level, const RuntimeSessionBuildConfig &config) const
{
	RuntimeSessionBuildResult result;
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
	result.state.renderCache = result.renderCache.state;
	result.state.tickIndex = 0;
	result.state.hasRenderCache = true;
	ApplyPlayerConfig(result.state, config);
	return result;
}

} // namespace iggy::runtime
