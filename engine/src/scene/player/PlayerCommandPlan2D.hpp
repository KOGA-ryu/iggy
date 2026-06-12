#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/player/PlayerAgentState.hpp"

namespace iggy {

enum class PlayerCommandPlan2DType {
	None,
	MoveToPoint,
	Interact,
	Wait,
	Rejected,
};

enum class PlayerCommandPlan2DRejectReason {
	None,
	InvalidCommand,
	ActorMismatch,
};

struct PlayerCommandPlan2D {
	PlayerCommandPlan2DType type = PlayerCommandPlan2DType::None;
	PlayerCommandPlan2DRejectReason rejectReason = PlayerCommandPlan2DRejectReason::None;
	ResourceId actorId;
	Vec2 targetPoint;
	TileCoord targetTile;
	ResourceId targetId;
};

} // namespace iggy
