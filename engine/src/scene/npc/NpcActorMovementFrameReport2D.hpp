#pragma once

#include <cstddef>
#include <vector>

#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorMovementFrameApply2D.hpp"

namespace iggy {

enum class NpcActorMovementFrameEvent2D {
	MovementApplied,
	MovementBlocked,
	MovementRejected,
	MovementNoOp,
	ActorNotFound,
	DirtyTileObserved,
	RefreshNeeded,
	MovementFrameChanged,
	MovementFrameUnchanged,
};

struct NpcActorMovementFrameReport2D {
	NpcActorMovementFrameApply2DResult apply;
	NpcActorState2DRegistry registry;
	std::size_t requestCount = 0;
	std::size_t entryCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedCount = 0;
	std::size_t rejectedCount = 0;
	std::size_t noMovementCount = 0;
	std::size_t missingActorCount = 0;
	std::size_t changedCount = 0;
	std::vector<TileCoord> dirtyTiles;
	bool needsOccupancyRebuild = false;
	bool needsAiMapQueryRefresh = false;
	bool needsInteractionRefresh = false;
	bool needsRenderRefresh = false;
	bool needsVisibilityRefresh = false;
	std::vector<NpcActorMovementFrameEvent2D> events;

	[[nodiscard]] bool changed() const;
	[[nodiscard]] bool hasDirtyTiles() const;
};

class NpcActorMovementFrameReporter2D {
public:
	[[nodiscard]] NpcActorMovementFrameReport2D report(
		const NpcActorMovementFrameApply2DResult &apply) const;
};

} // namespace iggy
