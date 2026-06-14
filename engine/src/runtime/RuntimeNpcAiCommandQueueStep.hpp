#pragma once

#include "runtime/RuntimeCommandQueue.hpp"
#include "scene/ai/NpcAiCommandFrameMapper2D.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiCommandQueueStatus {
	Queued,
	RejectedFull,
};

struct RuntimeNpcAiCommandQueueResult {
	RuntimeNpcAiCommandQueueStatus status = RuntimeNpcAiCommandQueueStatus::Queued;
	NpcAiCommandFrameMapper2DResult mapping;
	RuntimeCommandQueuePushResult push;
	RuntimeCommandQueueState queue;
};

class RuntimeNpcAiCommandQueueStep {
public:
	[[nodiscard]] RuntimeNpcAiCommandQueueResult push(
		const RuntimeCommandQueueState &queue,
		const RuntimeCommandQueueConfig &queueConfig,
		const NpcAiCommandFrameMapper2DResult &mapping) const;
};

} // namespace iggy::runtime
