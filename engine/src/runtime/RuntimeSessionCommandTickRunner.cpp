#include "runtime/RuntimeSessionCommandTickRunner.hpp"

namespace iggy::runtime {

RuntimeSessionCommandTickRunnerResult RuntimeSessionCommandTickRunner::run(
	const RuntimeSessionCommandTickRunnerInput &input,
	const physics2d::CollisionWorld2D &collisionWorld) const
{
	RuntimeSessionCommandTickRunnerResult result;
	result.session = input.session;
	result.ticks.reserve(input.commandFrames.size());

	for (const GameplayCommandFrame2D &commandFrame : input.commandFrames) {
		RuntimeSessionCommandTickResult tick = RuntimeSessionCommandTick {}.run({
			result.session,
			commandFrame,
			input.fallbackPlayerPosition,
			input.playerCommandConfig,
			input.npcConfig,
		},
			collisionWorld);
		result.session = tick.session;
		result.ticks.push_back(tick);
	}

	return result;
}

} // namespace iggy::runtime
