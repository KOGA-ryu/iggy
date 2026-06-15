#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcAiMovementRefreshFrameStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"
#include "scene/npc/NpcActorMovementRefreshFrame2D.hpp"

namespace iggy::runtime {

struct RuntimeNpcAiMovementRefreshFrameRunnerFrame {
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcMapPlayControlFramePlanConfig controlConfig;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
	InteractionTarget2DRegistry interactionTargets;
	AiMap2D refreshAiMap;
	NpcActorMovementRefreshFrame2DConfig refreshConfig;
};

struct RuntimeNpcAiMovementRefreshFrameRunnerInput {
	RuntimeGameplayState initialState;
	std::vector<RuntimeNpcAiMovementRefreshFrameRunnerFrame> frames;
};

struct RuntimeNpcAiMovementRefreshFrameRunnerResult {
	RuntimeGameplayState initialState;
	std::vector<RuntimeNpcAiMovementRefreshFrameRunnerFrame> frames;
	RuntimeGameplayState state;
	std::vector<RuntimeNpcAiMovementRefreshFrameResult> frameResults;
	std::size_t frameCount = 0;
	std::size_t controlPlannedRequestCount = 0;
	std::size_t controlAppliedCount = 0;
	std::size_t controlFailedCount = 0;
	std::size_t controlChangedFrameCount = 0;
	std::size_t movementPlannedRequestCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedMovementCount = 0;
	std::size_t rejectedMovementCount = 0;
	std::size_t missingActorMovementCount = 0;
	std::size_t actorChangedFrameCount = 0;
	std::size_t changedFrameCount = 0;
	std::size_t dirtyTileCount = 0;
	std::size_t occupancyRefreshCount = 0;
	std::size_t interactionRefreshCount = 0;
	std::size_t aiMapRefreshCount = 0;
	std::size_t renderRefreshCount = 0;
	std::size_t visibilityRefreshCount = 0;

	[[nodiscard]] bool hasFrames() const;
	[[nodiscard]] bool changed() const;
	[[nodiscard]] bool refreshedAny() const;
};

class RuntimeNpcAiMovementRefreshFrameRunner {
public:
	[[nodiscard]] RuntimeNpcAiMovementRefreshFrameRunnerResult run(
		const RuntimeNpcAiMovementRefreshFrameRunnerInput &input) const;
};

} // namespace iggy::runtime
