#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeCommandQueue.hpp"
#include "scene/player/PlayerInputGatedCommandFrameMapper2D.hpp"
#include "scene/player/PlayerInputCommandFrameMapper2D.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputQueueStatus {
	Queued,
	RejectedFull,
};

struct RuntimePlayerInputQueueResult {
	RuntimePlayerInputQueueStatus status = RuntimePlayerInputQueueStatus::Queued;
	RuntimeCommandQueueState queue;
	PlayerInputCommandFrameMapper2DResult mapping;
	RuntimeCommandQueuePushResult push;
};

struct RuntimePlayerInputQueueGatedResult {
	RuntimePlayerInputQueueStatus status = RuntimePlayerInputQueueStatus::Queued;
	RuntimeCommandQueueState queue;
	PlayerInputGatedCommandFrameMapper2DResult mapping;
	RuntimeCommandQueuePushResult push;
};

class RuntimePlayerInputQueueStep {
public:
	[[nodiscard]] RuntimePlayerInputQueueResult push(
		const RuntimeCommandQueueState &queue,
		const RuntimeCommandQueueConfig &queueConfig,
		ResourceId actorId,
		const std::vector<PlayerInputIntent2D> &intents) const;

	[[nodiscard]] RuntimePlayerInputQueueGatedResult pushGated(
		const RuntimeCommandQueueState &queue,
		const RuntimeCommandQueueConfig &queueConfig,
		ResourceId actorId,
		const PlayerInputContext2D &context,
		const std::vector<PlayerInputIntent2D> &intents) const;
};

} // namespace iggy::runtime
