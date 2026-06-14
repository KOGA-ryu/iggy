#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeNpcAiMovementQueueStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcAiDecision2D.hpp"
#include "scene/ai/NpcAiMovementProposal2D.hpp"
#include "scene/ai/NpcAiNavigationRequest2D.hpp"
#include "scene/ai/NpcAiPathReport2D.hpp"
#include "scene/ai/NpcAiProfile2D.hpp"
#include "scene/ai/NpcAiRouteRequest2D.hpp"
#include "scene/level/LevelTileMap.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiDecisionQueueStatus {
	Queued,
	RejectedFull,
};

struct RuntimeNpcAiDecisionQueueNpcInput {
	NpcAiProfile2D profile;
	NpcAiCurrentState2D state;
};

struct RuntimeNpcAiDecisionQueueConfig {
	NpcAiDecision2DConfig decision;
	NpcAiRouteRequest2DConfig route;
	NpcAiMovementProposal2DConfig proposal;
};

struct RuntimeNpcAiDecisionQueueEntry {
	std::size_t npcIndex = 0;
	NpcAiProfile2D profile;
	NpcAiCurrentState2D state;
	NpcAiDecision2DResult decision;
	NpcAiRouteRequest2DResult route;
	NpcAiNavigationRequest2DResult navigation;
	NpcAiPathReport2DResult path;
	NpcAiMovementProposal2DResult proposal;
};

struct RuntimeNpcAiDecisionQueueResult {
	RuntimeNpcAiDecisionQueueStatus status = RuntimeNpcAiDecisionQueueStatus::Queued;
	std::vector<RuntimeNpcAiDecisionQueueEntry> entries;
	NpcAiCommandFrameMapper2DResult mapping;
	RuntimeNpcAiMovementQueueResult queuePush;
	RuntimeCommandQueueState queue;
};

class RuntimeNpcAiDecisionQueueStep {
public:
	[[nodiscard]] RuntimeNpcAiDecisionQueueResult push(
		const RuntimeCommandQueueState &queue,
		const RuntimeCommandQueueConfig &queueConfig,
		const AiMap2D &aiMap,
		const LevelTileMap &levelMap,
		const std::vector<RuntimeNpcAiDecisionQueueNpcInput> &npcs,
		const RuntimeNpcAiDecisionQueueConfig &config = {}) const;
};

} // namespace iggy::runtime
