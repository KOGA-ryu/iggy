#include "runtime/RuntimeQueuedMutationCommandRunner.hpp"

namespace iggy::runtime {
namespace {

RuntimeSessionMutationCommandRunnerInput RunnerInput(
	const RuntimeQueuedMutationCommandRunnerInput &input,
	const RuntimeMutationCommandQueueDrainResult &drained)
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

RuntimeQueuedMutationCommandRunnerResult RuntimeQueuedMutationCommandRunner::run(const RuntimeQueuedMutationCommandRunnerInput &input) const
{
	RuntimeQueuedMutationCommandRunnerResult result;
	result.drained = RuntimeMutationCommandQueue {}.drain(input.queue);
	result.queue = result.drained.queue;
	result.runner = RuntimeSessionMutationCommandRunner {}.run(RunnerInput(input, result.drained));
	result.session = result.runner.session;
	return result;
}

RuntimeQueuedMutationCommandRunnerResult RuntimeQueuedMutationCommandRunner::run(
	const RuntimeQueuedMutationCommandRunnerInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimeQueuedMutationCommandRunnerResult result;
	result.drained = RuntimeMutationCommandQueue {}.drain(input.queue);
	result.queue = result.drained.queue;
	result.runner = RuntimeSessionMutationCommandRunner {}.run(RunnerInput(input, result.drained), explicitWorld);
	result.session = result.runner.session;
	return result;
}

} // namespace iggy::runtime
