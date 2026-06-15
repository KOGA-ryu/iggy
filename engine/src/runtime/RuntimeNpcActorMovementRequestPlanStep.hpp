#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"

namespace iggy::runtime {

struct RuntimeNpcActorMovementRequestPlanInput {
	RuntimeGameplayState state;
	LevelTileMap map;
	NpcActorMovementFramePlan2DConfig config;
};

struct RuntimeNpcActorMovementRequestPlanResult {
	RuntimeGameplayState state;
	LevelTileMap map;
	NpcActorMovementFramePlan2DResult plan;
	std::vector<NpcActorMovementFrameApply2DRequest> requests;
	std::size_t requestCount = 0;
	std::size_t preparedCount = 0;
	std::size_t blockedRequestCount = 0;
	std::size_t noMovementIntentCount = 0;
	std::size_t routeFailedCount = 0;
	std::size_t escapeRouteFailedCount = 0;
	std::size_t navigationFailedCount = 0;
	std::size_t pathFailedCount = 0;
	std::size_t stepNotProposedCount = 0;

	[[nodiscard]] bool hasRequests() const;
};

class RuntimeNpcActorMovementRequestPlanStep {
public:
	[[nodiscard]] RuntimeNpcActorMovementRequestPlanResult plan(
		const RuntimeNpcActorMovementRequestPlanInput &input) const;
};

} // namespace iggy::runtime
