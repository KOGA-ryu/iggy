#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcAiMovementPlannedFrameStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/npc/NpcActorMovementRefreshFrame2D.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiMovementRefreshFrameStatus {
	Ran,
	NoChanges,
};

struct RuntimeNpcAiMovementRefreshFrameInput {
	RuntimeNpcAiMovementPlannedFrameInput planned;
	NpcActorOccupancy2D previousOccupancy;
	InteractionTarget2DRegistry interactionTargets;
	AiMap2D refreshAiMap;
	NpcActorMovementRefreshFrame2DConfig refreshConfig;
};

struct RuntimeNpcAiMovementRefreshFrameResult {
	RuntimeNpcAiMovementRefreshFrameStatus status =
		RuntimeNpcAiMovementRefreshFrameStatus::NoChanges;
	RuntimeNpcAiMovementRefreshFrameInput input;
	RuntimeNpcAiMovementPlannedFrameResult planned;
	NpcActorMovementRefreshFrame2DResult refresh;
	RuntimeGameplayState state;
	std::size_t controlPlannedRequestCount = 0;
	std::size_t controlAppliedCount = 0;
	std::size_t controlFailedCount = 0;
	std::size_t movementPlannedRequestCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedMovementCount = 0;
	std::size_t rejectedMovementCount = 0;
	std::size_t missingActorMovementCount = 0;
	std::size_t dirtyTileCount = 0;
	bool changedControls = false;
	bool changedActors = false;
	bool occupancyRefreshed = false;
	bool interactionRefreshed = false;
	bool aiMapRefreshed = false;
	bool renderRefreshed = false;
	bool visibilityRefreshed = false;

	[[nodiscard]] bool changedState() const;
	[[nodiscard]] bool refreshedAny() const;
};

class RuntimeNpcAiMovementRefreshFrameStep {
public:
	[[nodiscard]] RuntimeNpcAiMovementRefreshFrameResult run(
		const RuntimeNpcAiMovementRefreshFrameInput &input) const;
};

} // namespace iggy::runtime
