#include "runtime/RuntimePlayerNpcAiCommandFrameStep.hpp"

namespace iggy::runtime {
namespace {

RuntimeQueuedCommandRunnerInput RunnerInput(
	const RuntimePlayerNpcAiCommandFrameInput &input,
	const RuntimeCommandQueueState &queue)
{
	return {
		input.session,
		queue,
		input.fallbackPlayerPosition,
		input.playerCommandConfig,
		input.npcConfig,
	};
}

RuntimePlayerNpcAiCommandFrameStatus RejectedStatus(RuntimePlayerNpcAiQueueStatus status)
{
	return status == RuntimePlayerNpcAiQueueStatus::PlayerRejected
		? RuntimePlayerNpcAiCommandFrameStatus::PlayerRejected
		: RuntimePlayerNpcAiCommandFrameStatus::NpcAiRejected;
}

bool Rejected(RuntimePlayerNpcAiQueueStatus status)
{
	return status != RuntimePlayerNpcAiQueueStatus::Queued;
}

} // namespace

RuntimePlayerNpcAiCommandFrameResult RuntimePlayerNpcAiCommandFrameStep::run(
	const RuntimePlayerNpcAiCommandFrameInput &input) const
{
	RuntimePlayerNpcAiCommandFrameResult result;
	result.intake = RuntimePlayerNpcAiQueueStep {}.push(input.intake);

	if (Rejected(result.intake.status)) {
		result.status = RejectedStatus(result.intake.status);
		result.session = input.session;
		result.queue = result.intake.queue;
		return result;
	}

	result.runner = RuntimeQueuedCommandRunner {}.run(RunnerInput(input, result.intake.queue));
	result.session = result.runner.session;
	result.queue = result.runner.queue;
	return result;
}

RuntimePlayerNpcAiCommandFrameResult RuntimePlayerNpcAiCommandFrameStep::run(
	const RuntimePlayerNpcAiCommandFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePlayerNpcAiCommandFrameResult result;
	result.intake = RuntimePlayerNpcAiQueueStep {}.push(input.intake);

	if (Rejected(result.intake.status)) {
		result.status = RejectedStatus(result.intake.status);
		result.session = input.session;
		result.queue = result.intake.queue;
		return result;
	}

	result.runner = RuntimeQueuedCommandRunner {}.run(RunnerInput(input, result.intake.queue), explicitWorld);
	result.session = result.runner.session;
	result.queue = result.runner.queue;
	return result;
}

} // namespace iggy::runtime
