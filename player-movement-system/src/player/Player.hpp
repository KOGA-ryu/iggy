#pragma once

#include "player/ActorPosition.hpp"
#include "player/AnimationLock.hpp"
#include "interaction/DestinationAction.hpp"
#include "player/MovementModifiers.hpp"
#include "player/PlayerState.hpp"
#include "player/WalkPath.hpp"

namespace dev {

struct Player {
	ActorPosition position;
	PlayerMoveState moveState = PlayerMoveState::Idle;
	WalkPath path;
	DestinationAction destinationAction;
	MovementModifiers movementModifiers;
	AnimationLock animationLock;
	float moveSpeedTilesPerSecond = 4.0F;
};

} // namespace dev
