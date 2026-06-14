#include "scene/npc/NpcActorMovementIntent2D.hpp"

namespace {

bool KnownMoveMode(iggy::NpcMoveMode mode)
{
	switch (mode) {
	case iggy::NpcMoveMode::None:
	case iggy::NpcMoveMode::Still:
	case iggy::NpcMoveMode::Walk:
	case iggy::NpcMoveMode::Jog:
	case iggy::NpcMoveMode::Run:
	case iggy::NpcMoveMode::Sprint:
		return true;
	}
	return false;
}

void PreserveFrameFacts(iggy::NpcActorMovementIntent2D &intent, const iggy::NpcActorFrameState2D &frame)
{
	intent.frame = frame;
	intent.npcId = frame.actor.npcId;
	intent.startPosition = frame.actor.position;
	intent.moveMode = frame.control.moveMode;
	intent.speedMultiplier = iggy::npcMoveModeSpeedMultiplier(frame.control.moveMode);
}

} // namespace

namespace iggy {

bool NpcActorMovementIntent2D::ready() const
{
	return status == NpcActorMovementIntent2DStatus::Ready && requestsMovement;
}

NpcActorMovementIntent2D NpcActorMovementIntentProjector2D::project(
	const NpcActorFrameState2D &frame,
	const NpcActorMovementIntent2DConfig &config) const
{
	(void)config;

	NpcActorMovementIntent2D intent;
	PreserveFrameFacts(intent, frame);

	if (!frame.hasControl) {
		intent.status = NpcActorMovementIntent2DStatus::MissingControl;
		return intent;
	}

	if (!frame.actor.present) {
		intent.status = NpcActorMovementIntent2DStatus::ActorNotPresent;
		return intent;
	}

	if (!KnownMoveMode(frame.control.moveMode)) {
		intent.status = NpcActorMovementIntent2DStatus::InvalidMoveMode;
		return intent;
	}

	if (!npcMoveModeMoves(frame.control.moveMode)) {
		intent.status = NpcActorMovementIntent2DStatus::NoMovement;
		return intent;
	}

	const NpcBehaviorState &behavior = frame.control.behavior;
	switch (behavior.type) {
	case NpcBehaviorStateType::Seeking:
		intent.status = NpcActorMovementIntent2DStatus::Ready;
		intent.type = NpcActorMovementIntent2DType::MoveTo;
		intent.targetPosition = behavior.targetPosition;
		intent.requestsMovement = true;
		return intent;
	case NpcBehaviorStateType::Fleeing:
		intent.status = NpcActorMovementIntent2DStatus::Ready;
		intent.type = NpcActorMovementIntent2DType::MoveAwayFrom;
		intent.targetPosition = behavior.targetPosition;
		intent.requestsMovement = true;
		return intent;
	case NpcBehaviorStateType::Attacking:
	case NpcBehaviorStateType::Interacting:
		intent.status = NpcActorMovementIntent2DStatus::UnsupportedBehavior;
		return intent;
	case NpcBehaviorStateType::None:
	case NpcBehaviorStateType::Idle:
	case NpcBehaviorStateType::Waiting:
	case NpcBehaviorStateType::Stunned:
	case NpcBehaviorStateType::Disabled:
		intent.status = NpcActorMovementIntent2DStatus::NoMovement;
		return intent;
	}

	intent.status = NpcActorMovementIntent2DStatus::UnsupportedBehavior;
	return intent;
}

} // namespace iggy
