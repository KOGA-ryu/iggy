#include "runtime/RuntimeTick.hpp"

#include "scene/level/LevelRuntimeUpdate.hpp"

namespace iggy::runtime {

RuntimeTickResult RuntimeTick::run(const RuntimeTickInput &input) const
{
	const LevelRuntimeUpdateResult update = LevelRuntimeUpdate {}.updateNpcAgents(input.state, input.playerPosition, input.npcConfig);
	return { update.state, update.npcReports };
}

} // namespace iggy::runtime
