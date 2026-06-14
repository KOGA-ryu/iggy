#pragma once

#include "scene/npc/NpcActorPathStepOccupancyFilter2D.hpp"
#include "scene/npc/NpcActorPostMoveReport2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

enum class NpcActorMovementExecutor2DStatus {
	Moved,
	NoMovement,
	BlockedByNpc,
	Rejected,
	ActorMismatch,
	ActorNotPresent,
};

struct NpcActorMovementExecutor2DResult {
	NpcActorState2D actor;
	NpcActorPathStepOccupancyFilter2D filter;
	NpcActorState2D resultActor;
	NpcActorPostMoveReport2D postMove;
	NpcActorMovementExecutor2DStatus status = NpcActorMovementExecutor2DStatus::NoMovement;
	bool changed = false;

	[[nodiscard]] bool moved() const;
};

class NpcActorMovementExecutor2D {
public:
	[[nodiscard]] NpcActorMovementExecutor2DResult execute(
		const NpcActorState2D &actor,
		const NpcActorPathStepOccupancyFilter2D &filter) const;
};

} // namespace iggy
