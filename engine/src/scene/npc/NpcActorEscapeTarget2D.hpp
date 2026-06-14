#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorMovementIntent2D.hpp"

namespace iggy {

enum class NpcActorEscapeTarget2DStatus {
	Ready,
	NoMovementIntent,
	NotMoveAwayFrom,
	ThreatAtActorPosition,
	NoCandidates,
	NoBetterCandidate,
	InvalidMapQuery,
};

struct NpcActorEscapeTargetCandidate2D {
	TileCoord tile;
	Vec2 position;
	float threatDistanceSquared = 0.0F;
	float actorTravelDistanceSquared = 0.0F;
};

struct NpcActorEscapeTarget2DConfig {
	int searchRadius = 1;
	bool requireBetterThanCurrent = true;
	float threatAtActorTolerance = 0.001F;
};

struct NpcActorEscapeTarget2D {
	NpcActorMovementIntent2D intent;
	NpcActorEscapeTarget2DStatus status = NpcActorEscapeTarget2DStatus::NoMovementIntent;
	ResourceId npcId;
	Vec2 startPosition;
	Vec2 threatPosition;
	Vec2 escapePosition;
	TileCoord startTile;
	TileCoord escapeTile;
	std::vector<NpcActorEscapeTargetCandidate2D> candidates;
	std::size_t selectedCandidateIndex = 0;
	bool hasEscapeTarget = false;

	[[nodiscard]] bool ready() const;
};

class NpcActorEscapeTargetProjector2D {
public:
	[[nodiscard]] NpcActorEscapeTarget2D project(
		const NpcActorMovementIntent2D &intent,
		const LevelTileMap &map,
		const NpcActorEscapeTarget2DConfig &config = {}) const;
};

} // namespace iggy
