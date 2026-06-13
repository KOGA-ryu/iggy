#include "runtime/RuntimePlayerInputQueueStep.hpp"

namespace iggy::runtime {

RuntimePlayerInputQueueResult RuntimePlayerInputQueueStep::push(
	const RuntimeCommandQueueState &queue,
	const RuntimeCommandQueueConfig &queueConfig,
	ResourceId actorId,
	const std::vector<PlayerInputIntent2D> &intents) const
{
	RuntimePlayerInputQueueResult result;
	result.mapping = PlayerInputCommandFrameMapper2D {}.map(actorId, intents);
	result.push = RuntimeCommandQueue {}.push(queue, result.mapping.frame, queueConfig);
	result.queue = result.push.queue;
	result.status = result.push.status == RuntimeCommandQueueStatus::Accepted
		? RuntimePlayerInputQueueStatus::Queued
		: RuntimePlayerInputQueueStatus::RejectedFull;
	return result;
}

} // namespace iggy::runtime
