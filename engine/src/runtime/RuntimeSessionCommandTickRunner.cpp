#include "runtime/RuntimeSessionCommandTickRunner.hpp"

namespace iggy::runtime {

namespace {

RuntimeSessionCommandTickInput TickInput(
	const RuntimeSessionCommandTickRunnerInput &input,
	const RuntimeSessionState &session,
	const GameplayCommandFrame2D &commandFrame)
{
	return {
		session,
		commandFrame,
		input.fallbackPlayerPosition,
		input.playerCommandConfig,
		input.npcConfig,
	};
}

} // namespace

RuntimeSessionCommandTickRunnerResult RuntimeSessionCommandTickRunner::run(const RuntimeSessionCommandTickRunnerInput &input) const
{
	RuntimeSessionCommandTickRunnerResult result;
	result.session = input.session;
	result.ticks.reserve(input.commandFrames.size());

	for (const GameplayCommandFrame2D &commandFrame : input.commandFrames) {
		RuntimeSessionCommandTickResult tick = RuntimeSessionCommandTick {}.run(TickInput(input, result.session, commandFrame));
		result.session = tick.session;
		result.ticks.push_back(tick);
	}

	return result;
}

RuntimeSessionCommandTickRunnerResult RuntimeSessionCommandTickRunner::run(
	const RuntimeSessionCommandTickRunnerInput &input,
	const physics2d::CollisionWorld2D &collisionWorld) const
{
	RuntimeSessionCommandTickRunnerResult result;
	result.session = input.session;
	result.ticks.reserve(input.commandFrames.size());

	for (const GameplayCommandFrame2D &commandFrame : input.commandFrames) {
		RuntimeSessionCommandTickResult tick = RuntimeSessionCommandTick {}.run(
			TickInput(input, result.session, commandFrame),
			collisionWorld);
		result.session = tick.session;
		result.ticks.push_back(tick);
	}

	return result;
}

} // namespace iggy::runtime
