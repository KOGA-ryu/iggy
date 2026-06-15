#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcAiMovementPlannedFrameStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"

namespace iggy::runtime {

struct RuntimeNpcAiMovementPlannedFrameRunnerFrame {
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcMapPlayControlFramePlanConfig controlConfig;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
};

struct RuntimeNpcAiMovementPlannedFrameRunnerInput {
	RuntimeGameplayState initialState;
	std::vector<RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames;
};

struct RuntimeNpcAiMovementPlannedFrameRunnerResult {
	RuntimeGameplayState initialState;
	std::vector<RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames;
	RuntimeGameplayState state;
	std::vector<RuntimeNpcAiMovementPlannedFrameResult> frameResults;
	std::size_t frameCount = 0;
	std::size_t controlPlannedRequestCount = 0;
	std::size_t controlAppliedCount = 0;
	std::size_t controlFailedCount = 0;
	std::size_t controlChangedFrameCount = 0;
	std::size_t controlMapChangedSelectionCount = 0;
	std::size_t movementPlannedRequestCount = 0;
	std::size_t movementPreReservationRequestCount = 0;
	std::size_t movementReservationAcceptedCount = 0;
	std::size_t movementReservationRejectedCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedMovementCount = 0;
	std::size_t rejectedMovementCount = 0;
	std::size_t missingActorMovementCount = 0;
	std::size_t actorChangedFrameCount = 0;
	std::size_t changedFrameCount = 0;
	bool needsOccupancyRebuild = false;
	bool needsAiMapQueryRefresh = false;
	bool needsInteractionRefresh = false;
	bool needsRenderRefresh = false;
	bool needsVisibilityRefresh = false;

	[[nodiscard]] bool hasFrames() const;
	[[nodiscard]] bool changed() const;
};

class RuntimeNpcAiMovementPlannedFrameRunner {
public:
	[[nodiscard]] RuntimeNpcAiMovementPlannedFrameRunnerResult run(
		const RuntimeNpcAiMovementPlannedFrameRunnerInput &input) const;
};

} // namespace iggy::runtime
