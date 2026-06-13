#pragma once

#include <cstddef>
#include <vector>

#include "runtime/GameplayCommand2D.hpp"

namespace iggy::runtime {

struct RuntimeCommandQueueState {
	std::vector<GameplayCommandFrame2D> frames;
};

enum class RuntimeCommandQueueStatus {
	Accepted,
	RejectedFull,
};

struct RuntimeCommandQueuePushResult {
	RuntimeCommandQueueStatus status = RuntimeCommandQueueStatus::Accepted;
	RuntimeCommandQueueState queue;
};

struct RuntimeCommandQueuePopResult {
	bool hasFrame = false;
	GameplayCommandFrame2D frame;
	RuntimeCommandQueueState queue;
};

struct RuntimeCommandQueueDrainResult {
	std::vector<GameplayCommandFrame2D> frames;
	RuntimeCommandQueueState queue;
};

struct RuntimeCommandQueueConfig {
	std::size_t maxFrames = 0;
};

class RuntimeCommandQueue {
public:
	[[nodiscard]] RuntimeCommandQueuePushResult push(
		const RuntimeCommandQueueState &state,
		const GameplayCommandFrame2D &frame,
		RuntimeCommandQueueConfig config = {}) const;

	[[nodiscard]] RuntimeCommandQueuePopResult pop(const RuntimeCommandQueueState &state) const;
	[[nodiscard]] RuntimeCommandQueueDrainResult drain(const RuntimeCommandQueueState &state) const;
};

} // namespace iggy::runtime
