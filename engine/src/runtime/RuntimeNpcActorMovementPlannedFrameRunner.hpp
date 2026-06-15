#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcActorMovementPlannedFrameStep.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"

namespace iggy::runtime {

struct RuntimeNpcActorMovementPlannedFrameRunnerFrame {
	LevelTileMap map;
	NpcActorMovementFramePlan2DConfig config;
};

struct RuntimeNpcActorMovementPlannedFrameRunnerInput {
	RuntimeGameplayState initialState;
	std::vector<RuntimeNpcActorMovementPlannedFrameRunnerFrame> frames;
};

struct RuntimeNpcActorMovementPlannedFrameRunnerResult {
	RuntimeGameplayState initialState;
	std::vector<RuntimeNpcActorMovementPlannedFrameRunnerFrame> frames;
	RuntimeGameplayState state;
	std::vector<RuntimeNpcActorMovementPlannedFrameResult> frameResults;
	std::size_t frameCount = 0;
	std::size_t plannedRequestCount = 0;
	std::size_t preReservationRequestCount = 0;
	std::size_t reservationAcceptedCount = 0;
	std::size_t reservationRejectedCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedMovementCount = 0;
	std::size_t rejectedMovementCount = 0;
	std::size_t missingActorMovementCount = 0;
	std::size_t changedFrameCount = 0;
	bool needsOccupancyRebuild = false;
	bool needsAiMapQueryRefresh = false;
	bool needsInteractionRefresh = false;
	bool needsRenderRefresh = false;
	bool needsVisibilityRefresh = false;

	[[nodiscard]] bool hasFrames() const;
	[[nodiscard]] bool changed() const;
};

class RuntimeNpcActorMovementPlannedFrameRunner {
public:
	[[nodiscard]] RuntimeNpcActorMovementPlannedFrameRunnerResult run(
		const RuntimeNpcActorMovementPlannedFrameRunnerInput &input) const;
};

} // namespace iggy::runtime
