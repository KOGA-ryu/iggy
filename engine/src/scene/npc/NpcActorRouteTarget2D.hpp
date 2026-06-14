#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/npc/NpcActorMovementIntent2D.hpp"
#include "scene/npc/NpcMoveMode.hpp"

namespace iggy {

enum class NpcActorRouteTarget2DStatus {
	Ready,
	NoMovementIntent,
	AlreadyAtTarget,
	NeedsEscapeDestination,
	NoEscapeDestination,
	InvalidTarget,
};

enum class NpcActorRouteTarget2DType {
	None,
	MoveTo,
	MoveAwayFrom,
};

struct NpcActorRouteTarget2DConfig {
	float arrivalTolerance = 0.001F;
	bool hasEscapeDestination = false;
	Vec2 escapeDestination;
};

struct NpcActorRouteTarget2D {
	NpcActorMovementIntent2D intent;
	NpcActorRouteTarget2DStatus status = NpcActorRouteTarget2DStatus::NoMovementIntent;
	NpcActorRouteTarget2DType type = NpcActorRouteTarget2DType::None;
	ResourceId npcId;
	Vec2 startPosition;
	Vec2 targetPosition;
	Vec2 sourcePosition;
	NpcMoveMode moveMode = NpcMoveMode::None;
	float speedMultiplier = 0.0F;
	bool requestsRoute = false;

	[[nodiscard]] bool ready() const;
};

class NpcActorRouteTargetProjector2D {
public:
	[[nodiscard]] NpcActorRouteTarget2D project(
		const NpcActorMovementIntent2D &intent,
		const NpcActorRouteTarget2DConfig &config = {}) const;
};

} // namespace iggy
