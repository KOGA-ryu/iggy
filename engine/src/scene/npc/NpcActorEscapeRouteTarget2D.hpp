#pragma once

#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorEscapeTarget2D.hpp"
#include "scene/npc/NpcActorMovementIntent2D.hpp"
#include "scene/npc/NpcActorRouteTarget2D.hpp"

namespace iggy {

enum class NpcActorEscapeRouteTarget2DStatus {
	Ready,
	NoMovementIntent,
	NotMoveAwayFrom,
	EscapeTargetFailed,
	RouteTargetFailed,
};

struct NpcActorEscapeRouteTarget2DConfig {
	NpcActorEscapeTarget2DConfig escape;
	NpcActorRouteTarget2DConfig route;
};

struct NpcActorEscapeRouteTarget2D {
	NpcActorMovementIntent2D intent;
	NpcActorEscapeTarget2D escape;
	NpcActorRouteTarget2D route;
	NpcActorEscapeRouteTarget2DStatus status = NpcActorEscapeRouteTarget2DStatus::NoMovementIntent;
	bool requestsRoute = false;

	[[nodiscard]] bool ready() const;
};

class NpcActorEscapeRouteTargetProjector2D {
public:
	[[nodiscard]] NpcActorEscapeRouteTarget2D project(
		const NpcActorMovementIntent2D &intent,
		const LevelTileMap &map,
		const NpcActorEscapeRouteTarget2DConfig &config = {}) const;
};

} // namespace iggy
