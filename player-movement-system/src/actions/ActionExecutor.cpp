#include "ActionExecutor.hpp"

namespace dev {

ActionExecutor::ActionExecutor(ActionRules rules)
    : rules_(rules)
{
}

ActionResult ActionExecutor::update(Player &player) const
{
	const DestinationAction action = player.destinationAction;
	if (action.type == DestinationActionType::None)
		return { ActionResultType::NoAction, false };

	if (!rules_.canExecute(player, action))
		return { ActionResultType::BlockedByState, false };

	if (!rules_.targetStillValid(action))
		return { ActionResultType::InvalidTarget, true };

	if (!rules_.targetInRange(player, action))
		return { ActionResultType::OutOfRange, false };

	applyAnimationCommitment(player, action);
	player.destinationAction = {};
	player.moveState = PlayerMoveState::Idle;
	return { ActionResultType::Executed, true };
}

void ActionExecutor::applyAnimationCommitment(Player &player, const DestinationAction &action) const
{
	player.animationLock.active = true;
	player.animationLock.elapsedSeconds = 0.0F;

	switch (action.type) {
	case DestinationActionType::Attack:
		player.animationLock.cancelAfterSeconds = 0.18F;
		break;
	case DestinationActionType::Pickup:
		player.animationLock.cancelAfterSeconds = 0.08F;
		break;
	case DestinationActionType::Talk:
	case DestinationActionType::Interact:
		player.animationLock.cancelAfterSeconds = 0.12F;
		break;
	case DestinationActionType::None:
		player.animationLock.active = false;
		player.animationLock.cancelAfterSeconds = 0.0F;
		break;
	}
}

} // namespace dev

