#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcActorMovementFrameStep.hpp"
#include "runtime/RuntimeNpcActorMovementRequestPlanStep.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"

namespace iggy::runtime {

enum class RuntimeNpcActorMovementPlannedFrameStatus {
	Ran,
	NoRequests,
};

struct RuntimeNpcActorMovementPlannedFrameInput {
	RuntimeGameplayState state;
	LevelTileMap map;
	NpcActorMovementFramePlan2DConfig config;
};

struct RuntimeNpcActorMovementPlannedFrameResult {
	RuntimeNpcActorMovementPlannedFrameStatus status =
		RuntimeNpcActorMovementPlannedFrameStatus::NoRequests;
	RuntimeGameplayState inputState;
	LevelTileMap map;
	NpcActorMovementFramePlan2DConfig config;
	RuntimeNpcActorMovementRequestPlanResult plan;
	RuntimeNpcActorMovementFrameResult movement;
	RuntimeGameplayState state;
	std::size_t plannedRequestCount = 0;
	std::size_t preReservationRequestCount = 0;
	std::size_t reservationAcceptedCount = 0;
	std::size_t reservationRejectedCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedCount = 0;
	std::size_t rejectedCount = 0;
	std::size_t missingActorCount = 0;
	bool changed = false;

	[[nodiscard]] bool ranMovementRequests() const;
	[[nodiscard]] bool changedState() const;
};

class RuntimeNpcActorMovementPlannedFrameStep {
public:
	[[nodiscard]] RuntimeNpcActorMovementPlannedFrameResult run(
		const RuntimeNpcActorMovementPlannedFrameInput &input) const;
};

} // namespace iggy::runtime
