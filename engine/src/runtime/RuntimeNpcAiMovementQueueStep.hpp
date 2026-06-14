#pragma once

#include <vector>

#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeNpcAiCommandQueueStep.hpp"
#include "scene/ai/NpcAiCommandFrameMapper2D.hpp"
#include "scene/ai/NpcAiMovementProposal2D.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiMovementQueueStatus {
	Queued,
	RejectedFull,
};

struct RuntimeNpcAiMovementQueueResult {
	RuntimeNpcAiMovementQueueStatus status = RuntimeNpcAiMovementQueueStatus::Queued;
	NpcAiCommandFrameMapper2DResult mapping;
	RuntimeNpcAiCommandQueueResult queuePush;
	RuntimeCommandQueueState queue;
};

class RuntimeNpcAiMovementQueueStep {
public:
	[[nodiscard]] RuntimeNpcAiMovementQueueResult push(
		const RuntimeCommandQueueState &queue,
		const RuntimeCommandQueueConfig &queueConfig,
		const std::vector<NpcAiMovementProposal2DResult> &proposals) const;
};

} // namespace iggy::runtime
