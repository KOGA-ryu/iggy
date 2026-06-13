#include "scene/player/PlayerInputCommandMapper2D.hpp"

namespace iggy {

PlayerInputCommandMapper2DResult PlayerInputCommandMapper2D::map(ResourceId actorId, PlayerInputIntent2D intent) const
{
	PlayerInputCommandMapper2DResult result;
	result.intentStatus = validate(intent);
	if (result.intentStatus != PlayerInputIntent2DStatus::Valid) {
		result.status = PlayerInputCommandMapper2DStatus::InvalidIntent;
		return result;
	}

	const runtime::GameplayCommand2DFactory factory;
	switch (intent.type) {
	case PlayerInputIntent2DType::None:
		result.command = factory.none(actorId);
		break;
	case PlayerInputIntent2DType::MoveToPoint:
		result.command = factory.moveToPoint(actorId, intent.worldPoint);
		break;
	case PlayerInputIntent2DType::MoveToTile:
		result.command = factory.moveToTile(actorId, intent.tile);
		break;
	case PlayerInputIntent2DType::Interact:
		result.command = factory.interact(actorId, intent.targetId);
		break;
	case PlayerInputIntent2DType::Wait:
		result.command = factory.wait(actorId);
		break;
	case PlayerInputIntent2DType::Inspect:
	case PlayerInputIntent2DType::Cancel:
		result.status = PlayerInputCommandMapper2DStatus::UnsupportedIntent;
		return result;
	}

	result.status = PlayerInputCommandMapper2DStatus::Mapped;
	return result;
}

} // namespace iggy
