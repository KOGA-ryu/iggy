#include "scene/player/PlayerCommandPlanner2D.hpp"

namespace iggy {

namespace {

ResourceId ResolveActorId(const PlayerAgentState &player, const runtime::GameplayCommand2D &command)
{
	if (!command.actorId.empty())
		return command.actorId;
	return player.id;
}

bool ActorMismatch(const PlayerAgentState &player, const runtime::GameplayCommand2D &command)
{
	return !command.actorId.empty() && !player.id.empty() && command.actorId != player.id;
}

} // namespace

PlayerCommandPlan2D PlayerCommandPlanner2D::plan(const PlayerAgentState &player, const runtime::GameplayCommand2D &command) const
{
	PlayerCommandPlan2D plan;
	plan.actorId = ResolveActorId(player, command);

	if (runtime::validate(command) != runtime::GameplayCommand2DStatus::Valid) {
		plan.type = PlayerCommandPlan2DType::Rejected;
		plan.rejectReason = PlayerCommandPlan2DRejectReason::InvalidCommand;
		return plan;
	}

	if (ActorMismatch(player, command)) {
		plan.type = PlayerCommandPlan2DType::Rejected;
		plan.rejectReason = PlayerCommandPlan2DRejectReason::ActorMismatch;
		return plan;
	}

	switch (command.type) {
	case runtime::GameplayCommand2DType::None:
		plan.type = PlayerCommandPlan2DType::None;
		return plan;
	case runtime::GameplayCommand2DType::Wait:
		plan.type = PlayerCommandPlan2DType::Wait;
		return plan;
	case runtime::GameplayCommand2DType::MoveToPoint:
		plan.type = PlayerCommandPlan2DType::MoveToPoint;
		plan.targetPoint = command.targetPoint;
		plan.targetTile = tileForPoint(command.targetPoint);
		return plan;
	case runtime::GameplayCommand2DType::MoveToTile:
		plan.type = PlayerCommandPlan2DType::MoveToPoint;
		plan.targetTile = command.targetTile;
		plan.targetPoint = tileCenter(command.targetTile);
		return plan;
	case runtime::GameplayCommand2DType::Interact:
		plan.type = PlayerCommandPlan2DType::Interact;
		plan.targetId = command.targetId;
		return plan;
	}

	return plan;
}

} // namespace iggy
