#include "runtime/RuntimeCommandQueue.hpp"

namespace iggy::runtime {

RuntimeCommandQueuePushResult RuntimeCommandQueue::push(
	const RuntimeCommandQueueState &state,
	const GameplayCommandFrame2D &frame,
	RuntimeCommandQueueConfig config) const
{
	RuntimeCommandQueuePushResult result;
	if (config.maxFrames > 0 && state.frames.size() >= config.maxFrames) {
		result.status = RuntimeCommandQueueStatus::RejectedFull;
		result.queue = state;
		return result;
	}

	result.status = RuntimeCommandQueueStatus::Accepted;
	result.queue = state;
	result.queue.frames.push_back(frame);
	return result;
}

RuntimeCommandQueuePopResult RuntimeCommandQueue::pop(const RuntimeCommandQueueState &state) const
{
	RuntimeCommandQueuePopResult result;
	if (state.frames.empty()) {
		result.queue = state;
		return result;
	}

	result.hasFrame = true;
	result.frame = state.frames.front();
	result.queue.frames.assign(state.frames.begin() + 1, state.frames.end());
	return result;
}

RuntimeCommandQueueDrainResult RuntimeCommandQueue::drain(const RuntimeCommandQueueState &state) const
{
	RuntimeCommandQueueDrainResult result;
	result.frames = state.frames;
	return result;
}

} // namespace iggy::runtime
