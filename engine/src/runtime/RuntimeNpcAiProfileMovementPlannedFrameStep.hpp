#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcActorMovementPlannedFrameStep.hpp"
#include "runtime/RuntimeNpcAiProfileControlPlannedFrameStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcAiProfileTraitResolver.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiProfileMovementPlannedFrameStatus {
	Ran,
	NoChanges,
};

struct RuntimeNpcAiProfileMovementPlannedFrameInput {
	RuntimeGameplayState state;
	NpcAiProfileTraitCatalog profileTraits;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcAiProfileTraitResolverConfig profileConfig;
	NpcMapPlayControlFramePlanConfig controlConfig;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
};

struct RuntimeNpcAiProfileMovementPlannedFrameResult {
	RuntimeNpcAiProfileMovementPlannedFrameStatus status =
		RuntimeNpcAiProfileMovementPlannedFrameStatus::NoChanges;
	RuntimeNpcAiProfileMovementPlannedFrameInput input;
	RuntimeNpcAiProfileControlPlannedFrameResult control;
	RuntimeNpcActorMovementPlannedFrameResult movement;
	RuntimeGameplayState state;
	std::size_t resolvedSubjectCount = 0;
	std::size_t missingProfileCount = 0;
	std::size_t emptyActorProfileIdCount = 0;
	std::size_t absentSkippedCount = 0;
	std::size_t controlPlannedRequestCount = 0;
	std::size_t controlAppliedCount = 0;
	std::size_t controlFailedCount = 0;
	std::size_t controlPlanIssueCount = 0;
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

class RuntimeNpcAiProfileMovementPlannedFrameStep {
public:
	[[nodiscard]] RuntimeNpcAiProfileMovementPlannedFrameResult run(
		const RuntimeNpcAiProfileMovementPlannedFrameInput &input) const;
};

} // namespace iggy::runtime
