#include "runtime/RuntimeMutationCommandQueue.hpp"

namespace iggy::runtime {

RuntimeMutationCommandQueuePushResult RuntimeMutationCommandQueue::push(
	const RuntimeMutationCommandQueueState &state,
	const RuntimeSessionMutationCommandFrame &frame,
	RuntimeMutationCommandQueueConfig config) const
{
	RuntimeMutationCommandQueuePushResult result;
	if (config.maxFrames > 0 && state.frames.size() >= config.maxFrames) {
		result.status = RuntimeMutationCommandQueueStatus::RejectedFull;
		result.queue = state;
		return result;
	}

	result.status = RuntimeMutationCommandQueueStatus::Accepted;
	result.queue = state;
	result.queue.frames.push_back(frame);
	return result;
}

RuntimeMutationCommandQueuePopResult RuntimeMutationCommandQueue::pop(const RuntimeMutationCommandQueueState &state) const
{
	RuntimeMutationCommandQueuePopResult result;
	if (state.frames.empty()) {
		result.queue = state;
		return result;
	}

	result.hasFrame = true;
	result.frame = state.frames.front();
	result.queue.frames.assign(state.frames.begin() + 1, state.frames.end());
	return result;
}

RuntimeMutationCommandQueueDrainResult RuntimeMutationCommandQueue::drain(const RuntimeMutationCommandQueueState &state) const
{
	RuntimeMutationCommandQueueDrainResult result;
	result.frames = state.frames;
	return result;
}

} // namespace iggy::runtime
