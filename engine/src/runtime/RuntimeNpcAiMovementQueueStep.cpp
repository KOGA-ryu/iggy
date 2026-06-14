#include "runtime/RuntimeNpcAiMovementQueueStep.hpp"

namespace iggy::runtime {

RuntimeNpcAiMovementQueueResult RuntimeNpcAiMovementQueueStep::push(
	const RuntimeCommandQueueState &queue,
	const RuntimeCommandQueueConfig &queueConfig,
	const std::vector<NpcAiMovementProposal2DResult> &proposals) const
{
	RuntimeNpcAiMovementQueueResult result;
	result.mapping = NpcAiCommandFrameMapper2D {}.map(proposals);
	result.queuePush = RuntimeNpcAiCommandQueueStep {}.push(queue, queueConfig, result.mapping);
	result.queue = result.queuePush.queue;
	result.status = result.queuePush.status == RuntimeNpcAiCommandQueueStatus::Queued
		? RuntimeNpcAiMovementQueueStatus::Queued
		: RuntimeNpcAiMovementQueueStatus::RejectedFull;
	return result;
}

} // namespace iggy::runtime
