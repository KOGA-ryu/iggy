#include "scene/player/PlayerInputIntent2D.hpp"

namespace iggy {

PlayerInputIntent2D playerMoveToPointIntent(Vec2 point)
{
	PlayerInputIntent2D intent;
	intent.type = PlayerInputIntent2DType::MoveToPoint;
	intent.worldPoint = point;
	return intent;
}

PlayerInputIntent2D playerMoveToTileIntent(TileCoord tile)
{
	PlayerInputIntent2D intent;
	intent.type = PlayerInputIntent2DType::MoveToTile;
	intent.tile = tile;
	return intent;
}

PlayerInputIntent2D playerInteractIntent(ResourceId targetId)
{
	PlayerInputIntent2D intent;
	intent.type = PlayerInputIntent2DType::Interact;
	intent.targetId = targetId;
	return intent;
}

PlayerInputIntent2D playerInspectIntent(ResourceId targetId)
{
	PlayerInputIntent2D intent;
	intent.type = PlayerInputIntent2DType::Inspect;
	intent.targetId = targetId;
	return intent;
}

PlayerInputIntent2D playerWaitIntent()
{
	PlayerInputIntent2D intent;
	intent.type = PlayerInputIntent2DType::Wait;
	return intent;
}

PlayerInputIntent2D playerCancelIntent()
{
	PlayerInputIntent2D intent;
	intent.type = PlayerInputIntent2DType::Cancel;
	return intent;
}

PlayerInputIntent2DStatus validate(PlayerInputIntent2D intent)
{
	if ((intent.type == PlayerInputIntent2DType::Interact || intent.type == PlayerInputIntent2DType::Inspect)
		&& intent.targetId.empty())
		return PlayerInputIntent2DStatus::MissingTarget;
	return PlayerInputIntent2DStatus::Valid;
}

bool valid(PlayerInputIntent2D intent)
{
	return validate(intent) == PlayerInputIntent2DStatus::Valid;
}

} // namespace iggy
