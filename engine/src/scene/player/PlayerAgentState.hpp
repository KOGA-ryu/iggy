#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

enum class PlayerMovementStatus {
	Idle,
	Moving,
};

enum class PlayerFacing2D {
	None,
	North,
	South,
	East,
	West,
};

struct PlayerAgentState {
	ResourceId id;
	Vec2 position;
	TileCoord spawnTile;
	PlayerMovementStatus movementStatus = PlayerMovementStatus::Idle;
	PlayerFacing2D facing = PlayerFacing2D::None;
};

[[nodiscard]] TileCoord playerTile(const PlayerAgentState &state);

} // namespace iggy
