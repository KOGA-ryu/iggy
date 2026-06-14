#include "runtime/RuntimePlayerNpcAiQueueStep.hpp"

namespace iggy::runtime {

namespace {

RuntimePlayerInputQueueGatedResult PushPlayer(
	const RuntimePlayerNpcAiQueueInput &input,
	const RuntimeCommandQueueState &queue)
{
	return RuntimePlayerInputQueueStep {}.pushGated(
		queue,
		input.queueConfig,
		input.playerActorId,
		input.playerInputContext,
		input.playerIntents);
}

RuntimeNpcAiDecisionQueueResult PushNpcAi(
	const RuntimePlayerNpcAiQueueInput &input,
	const RuntimeCommandQueueState &queue)
{
	return RuntimeNpcAiDecisionQueueStep {}.push(
		queue,
		input.queueConfig,
		input.aiMap,
		input.levelMap,
		input.npcs,
		input.npcAi);
}

} // namespace

RuntimePlayerNpcAiQueueResult RuntimePlayerNpcAiQueueStep::push(const RuntimePlayerNpcAiQueueInput &input) const
{
	RuntimePlayerNpcAiQueueResult result;
	result.order = input.order;

	if (input.order == RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi) {
		result.player = PushPlayer(input, input.queue);
		if (result.player.status == RuntimePlayerInputQueueStatus::RejectedFull) {
			result.status = RuntimePlayerNpcAiQueueStatus::PlayerRejected;
			result.queue = result.player.queue;
			return result;
		}

		result.npcAi = PushNpcAi(input, result.player.queue);
		if (result.npcAi.status == RuntimeNpcAiDecisionQueueStatus::RejectedFull) {
			result.status = RuntimePlayerNpcAiQueueStatus::NpcAiRejected;
			result.queue = result.npcAi.queue;
			return result;
		}

		result.queue = result.npcAi.queue;
		return result;
	}

	result.npcAi = PushNpcAi(input, input.queue);
	if (result.npcAi.status == RuntimeNpcAiDecisionQueueStatus::RejectedFull) {
		result.status = RuntimePlayerNpcAiQueueStatus::NpcAiRejected;
		result.queue = result.npcAi.queue;
		return result;
	}

	result.player = PushPlayer(input, result.npcAi.queue);
	if (result.player.status == RuntimePlayerInputQueueStatus::RejectedFull) {
		result.status = RuntimePlayerNpcAiQueueStatus::PlayerRejected;
		result.queue = result.player.queue;
		return result;
	}

	result.queue = result.player.queue;
	return result;
}

} // namespace iggy::runtime
