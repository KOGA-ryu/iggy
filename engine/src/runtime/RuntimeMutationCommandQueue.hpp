#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeSessionMutationCommandRunner.hpp"

namespace iggy::runtime {

struct RuntimeMutationCommandQueueState {
	std::vector<RuntimeSessionMutationCommandFrame> frames;
};

enum class RuntimeMutationCommandQueueStatus {
	Accepted,
	RejectedFull,
};

struct RuntimeMutationCommandQueuePushResult {
	RuntimeMutationCommandQueueStatus status = RuntimeMutationCommandQueueStatus::Accepted;
	RuntimeMutationCommandQueueState queue;
};

struct RuntimeMutationCommandQueuePopResult {
	bool hasFrame = false;
	RuntimeSessionMutationCommandFrame frame;
	RuntimeMutationCommandQueueState queue;
};

struct RuntimeMutationCommandQueueDrainResult {
	std::vector<RuntimeSessionMutationCommandFrame> frames;
	RuntimeMutationCommandQueueState queue;
};

struct RuntimeMutationCommandQueueConfig {
	std::size_t maxFrames = 0;
};

class RuntimeMutationCommandQueue {
public:
	[[nodiscard]] RuntimeMutationCommandQueuePushResult push(
		const RuntimeMutationCommandQueueState &state,
		const RuntimeSessionMutationCommandFrame &frame,
		RuntimeMutationCommandQueueConfig config = {}) const;

	[[nodiscard]] RuntimeMutationCommandQueuePopResult pop(const RuntimeMutationCommandQueueState &state) const;
	[[nodiscard]] RuntimeMutationCommandQueueDrainResult drain(const RuntimeMutationCommandQueueState &state) const;
};

} // namespace iggy::runtime
