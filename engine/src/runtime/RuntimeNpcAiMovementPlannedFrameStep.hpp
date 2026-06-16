#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcActorMovementPlannedFrameStep.hpp"
#include "runtime/RuntimeNpcAiControlPlannedFrameStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiMovementPlannedFrameStatus {
	Ran,
	NoChanges,
};

struct RuntimeNpcAiMovementPlannedFrameInput {
	RuntimeGameplayState state;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcMapPlayControlFramePlanConfig controlConfig;
	std::vector<NpcActorControlState2D> controlOverrides;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
};

struct RuntimeNpcAiMovementPlannedFrameResult {
	RuntimeNpcAiMovementPlannedFrameStatus status =
		RuntimeNpcAiMovementPlannedFrameStatus::NoChanges;
	RuntimeGameplayState inputState;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcMapPlayControlFramePlanConfig controlConfig;
	std::vector<NpcActorControlState2D> controlOverrides;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
	RuntimeNpcAiControlPlannedFrameResult control;
	RuntimeNpcActorMovementPlannedFrameResult movement;
	RuntimeGameplayState state;
	std::size_t controlPlannedRequestCount = 0;
	std::size_t controlAppliedCount = 0;
	std::size_t controlFailedCount = 0;
	std::size_t controlMapChangedSelectionCount = 0;
	std::size_t movementPlannedRequestCount = 0;
	std::size_t movementPreReservationRequestCount = 0;
	std::size_t movementReservationAcceptedCount = 0;
	std::size_t movementReservationRejectedCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedCount = 0;
	std::size_t rejectedCount = 0;
	std::size_t missingActorCount = 0;
	bool changedControls = false;
	bool changedActors = false;

	[[nodiscard]] bool ranAnyRequests() const;
	[[nodiscard]] bool changedState() const;
};

class RuntimeNpcAiMovementPlannedFrameStep {
public:
	[[nodiscard]] RuntimeNpcAiMovementPlannedFrameResult run(
		const RuntimeNpcAiMovementPlannedFrameInput &input) const;
};

} // namespace iggy::runtime
