#include "runtime/RuntimePlayerInputCommandRunner.hpp"

namespace iggy::runtime {
namespace {

RuntimeQueuedCommandRunnerInput QueuedRunnerInput(
	const RuntimePlayerInputCommandRunnerInput &input,
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

RuntimeQueuedCommandRunnerInput QueuedRunnerInput(
	const RuntimePlayerInputGatedCommandRunnerInput &input,
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

void SetRejectedResult(
	RuntimePlayerInputCommandRunnerResult &result,
	const RuntimePlayerInputCommandRunnerInput &input)
{
	result.status = RuntimePlayerInputCommandRunnerStatus::QueueRejected;
	result.session = input.session;
	result.queue = result.intake.queue;
}

void SetRejectedResult(
	RuntimePlayerInputGatedCommandRunnerResult &result,
	const RuntimePlayerInputGatedCommandRunnerInput &input)
{
	result.status = RuntimePlayerInputCommandRunnerStatus::QueueRejected;
	result.session = input.session;
	result.queue = result.intake.queue;
}

} // namespace

RuntimePlayerInputCommandRunnerResult RuntimePlayerInputCommandRunner::run(const RuntimePlayerInputCommandRunnerInput &input) const
{
	RuntimePlayerInputCommandRunnerResult result;
	result.intake = RuntimePlayerInputQueueStep {}.push(input.queue, input.queueConfig, input.actorId, input.intents);
	if (result.intake.status == RuntimePlayerInputQueueStatus::RejectedFull) {
		SetRejectedResult(result, input);
		return result;
	}

	result.runner = RuntimeQueuedCommandRunner {}.run(QueuedRunnerInput(input, result.intake.queue));
	result.status = RuntimePlayerInputCommandRunnerStatus::Ran;
	result.session = result.runner.session;
	result.queue = result.runner.queue;
	return result;
}

RuntimePlayerInputCommandRunnerResult RuntimePlayerInputCommandRunner::run(
	const RuntimePlayerInputCommandRunnerInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePlayerInputCommandRunnerResult result;
	result.intake = RuntimePlayerInputQueueStep {}.push(input.queue, input.queueConfig, input.actorId, input.intents);
	if (result.intake.status == RuntimePlayerInputQueueStatus::RejectedFull) {
		SetRejectedResult(result, input);
		return result;
	}

	result.runner = RuntimeQueuedCommandRunner {}.run(QueuedRunnerInput(input, result.intake.queue), explicitWorld);
	result.status = RuntimePlayerInputCommandRunnerStatus::Ran;
	result.session = result.runner.session;
	result.queue = result.runner.queue;
	return result;
}

RuntimePlayerInputGatedCommandRunnerResult RuntimePlayerInputCommandRunner::runGated(const RuntimePlayerInputGatedCommandRunnerInput &input) const
{
	RuntimePlayerInputGatedCommandRunnerResult result;
	result.intake = RuntimePlayerInputQueueStep {}.pushGated(input.queue, input.queueConfig, input.actorId, input.context, input.intents);
	if (result.intake.status == RuntimePlayerInputQueueStatus::RejectedFull) {
		SetRejectedResult(result, input);
		return result;
	}

	result.runner = RuntimeQueuedCommandRunner {}.run(QueuedRunnerInput(input, result.intake.queue));
	result.status = RuntimePlayerInputCommandRunnerStatus::Ran;
	result.session = result.runner.session;
	result.queue = result.runner.queue;
	return result;
}

RuntimePlayerInputGatedCommandRunnerResult RuntimePlayerInputCommandRunner::runGated(
	const RuntimePlayerInputGatedCommandRunnerInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePlayerInputGatedCommandRunnerResult result;
	result.intake = RuntimePlayerInputQueueStep {}.pushGated(input.queue, input.queueConfig, input.actorId, input.context, input.intents);
	if (result.intake.status == RuntimePlayerInputQueueStatus::RejectedFull) {
		SetRejectedResult(result, input);
		return result;
	}

	result.runner = RuntimeQueuedCommandRunner {}.run(QueuedRunnerInput(input, result.intake.queue), explicitWorld);
	result.status = RuntimePlayerInputCommandRunnerStatus::Ran;
	result.session = result.runner.session;
	result.queue = result.runner.queue;
	return result;
}

} // namespace iggy::runtime
