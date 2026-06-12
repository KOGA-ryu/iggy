#include "runtime/GameplayCommand2D.hpp"

namespace iggy::runtime {

GameplayCommand2D GameplayCommand2DFactory::none(ResourceId actorId) const
{
	GameplayCommand2D command;
	command.type = GameplayCommand2DType::None;
	command.actorId = actorId;
	return command;
}

GameplayCommand2D GameplayCommand2DFactory::moveToPoint(ResourceId actorId, Vec2 targetPoint) const
{
	GameplayCommand2D command;
	command.type = GameplayCommand2DType::MoveToPoint;
	command.actorId = actorId;
	command.targetPoint = targetPoint;
	return command;
}

GameplayCommand2D GameplayCommand2DFactory::moveToTile(ResourceId actorId, TileCoord targetTile) const
{
	GameplayCommand2D command;
	command.type = GameplayCommand2DType::MoveToTile;
	command.actorId = actorId;
	command.targetTile = targetTile;
	return command;
}

GameplayCommand2D GameplayCommand2DFactory::interact(ResourceId actorId, ResourceId targetId) const
{
	GameplayCommand2D command;
	command.type = GameplayCommand2DType::Interact;
	command.actorId = actorId;
	command.targetId = targetId;
	return command;
}

GameplayCommand2D GameplayCommand2DFactory::wait(ResourceId actorId) const
{
	GameplayCommand2D command;
	command.type = GameplayCommand2DType::Wait;
	command.actorId = actorId;
	return command;
}

GameplayCommand2DStatus validate(GameplayCommand2D command)
{
	if (command.type == GameplayCommand2DType::Interact && command.targetId.empty())
		return GameplayCommand2DStatus::MissingTarget;

	return GameplayCommand2DStatus::Valid;
}

bool valid(GameplayCommand2D command)
{
	return validate(command) == GameplayCommand2DStatus::Valid;
}

} // namespace iggy::runtime
