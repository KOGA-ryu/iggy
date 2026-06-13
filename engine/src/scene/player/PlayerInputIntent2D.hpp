#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

enum class PlayerInputIntent2DType {
	None,
	MoveToPoint,
	MoveToTile,
	Interact,
	Inspect,
	Wait,
	Cancel,
};

enum class PlayerInputIntent2DStatus {
	Valid,
	MissingTarget,
};

struct PlayerInputIntent2D {
	PlayerInputIntent2DType type = PlayerInputIntent2DType::None;
	Vec2 worldPoint;
	TileCoord tile;
	ResourceId targetId;
};

[[nodiscard]] PlayerInputIntent2D playerMoveToPointIntent(Vec2 point);
[[nodiscard]] PlayerInputIntent2D playerMoveToTileIntent(TileCoord tile);
[[nodiscard]] PlayerInputIntent2D playerInteractIntent(ResourceId targetId);
[[nodiscard]] PlayerInputIntent2D playerInspectIntent(ResourceId targetId);
[[nodiscard]] PlayerInputIntent2D playerWaitIntent();
[[nodiscard]] PlayerInputIntent2D playerCancelIntent();

[[nodiscard]] PlayerInputIntent2DStatus validate(PlayerInputIntent2D intent);
[[nodiscard]] bool valid(PlayerInputIntent2D intent);

} // namespace iggy
