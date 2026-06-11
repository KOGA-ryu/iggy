#include "PlayerAnimationLockGate.hpp"

namespace dev {

namespace {

void EmitAnimationUnlocked(MovementEventSink *eventSink, Point tile)
{
	if (eventSink == nullptr)
		return;
	eventSink->emit({
	    .type = MovementEventType::AnimationUnlocked,
	    .tile = tile,
	});
}

} // namespace

PlayerAnimationLockGate::PlayerAnimationLockGate(MovementEventSink *eventSink)
    : eventSink_(eventSink)
{
}

bool PlayerAnimationLockGate::advance(Player &player, float deltaSeconds) const
{
	if (!player.animationLock.active)
		return true;

	player.animationLock.elapsedSeconds += deltaSeconds;
	if (!player.animationLock.canCancel())
		return false;

	player.animationLock.active = false;
	EmitAnimationUnlocked(eventSink_, player.position.tile);
	return true;
}

} // namespace dev
