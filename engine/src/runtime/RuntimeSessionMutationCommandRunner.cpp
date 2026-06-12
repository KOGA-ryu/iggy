#include "runtime/RuntimeSessionMutationCommandRunner.hpp"

namespace iggy::runtime {

namespace {

RuntimeSessionMutationCommandInput StepInput(
	const RuntimeSessionMutationCommandRunnerInput &input,
	const RuntimeSessionState &session,
	const RuntimeSessionMutationCommandFrame &frame)
{
	return {
		session,
		frame.levelEdits,
		frame.commandFrame,
		input.fallbackPlayerPosition,
		input.playerCommandConfig,
		input.npcConfig,
	};
}

} // namespace

RuntimeSessionMutationCommandRunnerResult RuntimeSessionMutationCommandRunner::run(const RuntimeSessionMutationCommandRunnerInput &input) const
{
	RuntimeSessionMutationCommandRunnerResult result;
	result.session = input.session;
	result.ticks.reserve(input.frames.size());

	for (const RuntimeSessionMutationCommandFrame &frame : input.frames) {
		RuntimeSessionMutationCommandResult tick = RuntimeSessionMutationCommandStep {}.run(StepInput(input, result.session, frame));
		result.session = tick.session;
		result.ticks.push_back(tick);
	}

	return result;
}

RuntimeSessionMutationCommandRunnerResult RuntimeSessionMutationCommandRunner::run(
	const RuntimeSessionMutationCommandRunnerInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimeSessionMutationCommandRunnerResult result;
	result.session = input.session;
	result.ticks.reserve(input.frames.size());

	for (const RuntimeSessionMutationCommandFrame &frame : input.frames) {
		RuntimeSessionMutationCommandResult tick = RuntimeSessionMutationCommandStep {}.run(StepInput(input, result.session, frame), explicitWorld);
		result.session = tick.session;
		result.ticks.push_back(tick);
	}

	return result;
}

} // namespace iggy::runtime
