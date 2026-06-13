#include "runtime/RuntimeQueuedCommandRunner.hpp"

namespace iggy::runtime {
namespace {

RuntimeSessionCommandTickRunnerInput RunnerInput(
	const RuntimeQueuedCommandRunnerInput &input,
	const RuntimeCommandQueueDrainResult &drained)
{
	return {
		input.session,
		drained.frames,
		input.fallbackPlayerPosition,
		input.playerCommandConfig,
		input.npcConfig,
	};
}

} // namespace

RuntimeQueuedCommandRunnerResult RuntimeQueuedCommandRunner::run(const RuntimeQueuedCommandRunnerInput &input) const
{
	RuntimeQueuedCommandRunnerResult result;
	result.drained = RuntimeCommandQueue {}.drain(input.queue);
	result.queue = result.drained.queue;
	result.runner = RuntimeSessionCommandTickRunner {}.run(RunnerInput(input, result.drained));
	result.session = result.runner.session;
	return result;
}

RuntimeQueuedCommandRunnerResult RuntimeQueuedCommandRunner::run(
	const RuntimeQueuedCommandRunnerInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimeQueuedCommandRunnerResult result;
	result.drained = RuntimeCommandQueue {}.drain(input.queue);
	result.queue = result.drained.queue;
	result.runner = RuntimeSessionCommandTickRunner {}.run(RunnerInput(input, result.drained), explicitWorld);
	result.session = result.runner.session;
	return result;
}

} // namespace iggy::runtime
