#include "runtime/RuntimeNpcAiCommandQueueStep.hpp"

namespace iggy::runtime {

RuntimeNpcAiCommandQueueResult RuntimeNpcAiCommandQueueStep::push(
	const RuntimeCommandQueueState &queue,
	const RuntimeCommandQueueConfig &queueConfig,
	const NpcAiCommandFrameMapper2DResult &mapping) const
{
	RuntimeNpcAiCommandQueueResult result;
	result.mapping = mapping;
	result.push = RuntimeCommandQueue {}.push(queue, mapping.frame, queueConfig);
	result.queue = result.push.queue;
	result.status = result.push.status == RuntimeCommandQueueStatus::Accepted
		? RuntimeNpcAiCommandQueueStatus::Queued
		: RuntimeNpcAiCommandQueueStatus::RejectedFull;
	return result;
}

} // namespace iggy::runtime
