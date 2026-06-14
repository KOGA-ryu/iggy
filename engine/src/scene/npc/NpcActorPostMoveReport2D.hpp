#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

enum class NpcActorPostMoveReport2DStatus {
	NotMoved,
	Moved,
	Blocked,
	Rejected,
};

enum class NpcActorPostMoveBlockingKind2D {
	None,
	Map,
	Npc,
	CollisionWorld,
	InvalidStep,
};

struct NpcActorPostMoveReport2DInput {
	ResourceId npcId;
	Vec2 oldPosition;
	Vec2 newPosition;
	NpcActorPostMoveReport2DStatus status = NpcActorPostMoveReport2DStatus::NotMoved;
	NpcActorPostMoveBlockingKind2D blockingKind = NpcActorPostMoveBlockingKind2D::None;
	ResourceId blockingNpcId;
};

struct NpcActorPostMoveReport2D {
	ResourceId npcId;
	Vec2 oldPosition;
	Vec2 newPosition;
	TileCoord oldTile;
	TileCoord newTile;
	NpcActorPostMoveReport2DStatus status = NpcActorPostMoveReport2DStatus::NotMoved;
	NpcActorPostMoveBlockingKind2D blockingKind = NpcActorPostMoveBlockingKind2D::None;
	ResourceId blockingNpcId;
	std::vector<TileCoord> dirtyTiles;
	bool needsOccupancyRebuild = false;
	bool needsAiMapQueryRefresh = false;
	bool needsInteractionRefresh = false;
	bool needsRenderRefresh = false;
	bool needsVisibilityRefresh = false;

	[[nodiscard]] bool moved() const;
	[[nodiscard]] bool blocked() const;
	[[nodiscard]] bool hasDirtyTiles() const;
};

class NpcActorPostMoveReporter2D {
public:
	[[nodiscard]] NpcActorPostMoveReport2D report(const NpcActorPostMoveReport2DInput &input) const;

	[[nodiscard]] NpcActorPostMoveReport2D report(
		const ResourceId &npcId,
		Vec2 oldPosition,
		Vec2 newPosition,
		NpcActorPostMoveReport2DStatus status,
		NpcActorPostMoveBlockingKind2D blockingKind = NpcActorPostMoveBlockingKind2D::None,
		const ResourceId &blockingNpcId = {}) const;
};

} // namespace iggy
