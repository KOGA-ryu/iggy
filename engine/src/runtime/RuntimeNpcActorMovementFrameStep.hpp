#pragma once

#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"

namespace iggy::runtime {

struct RuntimeNpcActorMovementFrameInput {
	RuntimeGameplayState state;
	std::vector<NpcActorMovementFrameApply2DRequest> requests;
};

struct RuntimeNpcActorMovementFrameResult {
	RuntimeGameplayState state;
	NpcActorMovementFrameApply2DResult apply;
	NpcActorMovementFrameReport2D report;
	bool changed = false;
};

class RuntimeNpcActorMovementFrameStep {
public:
	[[nodiscard]] RuntimeNpcActorMovementFrameResult run(
		const RuntimeNpcActorMovementFrameInput &input) const;
};

} // namespace iggy::runtime
