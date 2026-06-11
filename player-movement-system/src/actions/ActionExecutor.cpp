#include "ActionExecutor.hpp"

namespace dev {

namespace {

void EmitActionEvent(MovementEventSink *eventSink, MovementEventType type, PlayerId playerId, const Player &player, const DestinationAction &action, ActionResultType result)
{
	if (eventSink == nullptr)
		return;
	eventSink->emit({
		.type = type,
		.playerId = playerId,
		.tile = player.position.tile,
		.actionType = action.type,
		.actionResult = result,
		.target = action.target,
	});
}

} // namespace

ActionExecutor::ActionExecutor(ActionRules rules, MovementEventSink *eventSink, CombatSystem *combatSystem)
    : rules_(rules)
    , eventSink_(eventSink)
    , combatSystem_(combatSystem)
{
}

ActionResult ActionExecutor::update(Player &player, PlayerId playerId) const
{
	const DestinationAction action = player.destinationAction;
	if (action.type == DestinationActionType::None)
		return { ActionResultType::NoAction, false };

	if (!rules_.canExecute(player, action)) {
		EmitActionEvent(eventSink_, MovementEventType::ActionRejected, playerId, player, action, ActionResultType::BlockedByState);
		return { ActionResultType::BlockedByState, false };
	}

	if (!rules_.targetStillValid(action)) {
		EmitActionEvent(eventSink_, MovementEventType::ActionRejected, playerId, player, action, ActionResultType::InvalidTarget);
		return { ActionResultType::InvalidTarget, true };
	}

	if (!rules_.targetInRange(player, action)) {
		EmitActionEvent(eventSink_, MovementEventType::ActionRejected, playerId, player, action, ActionResultType::OutOfRange);
		return { ActionResultType::OutOfRange, false };
	}

	if (action.type == DestinationActionType::Attack && combatSystem_ != nullptr) {
		const CombatResult combatResult = combatSystem_->resolvePlayerAttack(player, action);
		if (combatResult.type == CombatResultType::InvalidTarget) {
			EmitActionEvent(eventSink_, MovementEventType::ActionRejected, playerId, player, action, ActionResultType::InvalidTarget);
			return { ActionResultType::InvalidTarget, true };
		}
	}

	applyAnimationCommitment(player, action);
	player.destinationAction = {};
	player.moveState = PlayerMoveState::Idle;
	EmitActionEvent(eventSink_, MovementEventType::ActionExecuted, playerId, player, action, ActionResultType::Executed);
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
	if (player.animationLock.active && eventSink_ != nullptr) {
		eventSink_->emit({
			.type = MovementEventType::AnimationLocked,
			.tile = player.position.tile,
			.actionType = action.type,
		});
	}
}

} // namespace dev
