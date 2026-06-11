#pragma once

#include <optional>

#include "actions/ActionResult.hpp"
#include "commands/MovementCommand.hpp"
#include "enemies/EnemyAttackResult.hpp"
#include "enemies/EnemyPursuitResult.hpp"
#include "interaction/DestinationAction.hpp"
#include "targeting/Target.hpp"
#include "world/Point.hpp"

namespace dev {

enum class MovementEventType {
	CommandAccepted,
	CommandRejected,
	PathStarted,
	PathBlocked,
	StepCommitted,
	DestinationActionReady,
	ActionExecuted,
	ActionRejected,
	AnimationLocked,
	AnimationUnlocked,
	EnemyPursuitStopped,
	EnemyAttackTransitioned,
};

struct MovementEvent {
	MovementEventType type;
	PlayerId playerId = 0;
	Point tile;
	std::optional<MovementCommandType> commandType;
	std::optional<DestinationActionType> actionType;
	std::optional<ActionResultType> actionResult;
	std::optional<Target> target;
	std::optional<TargetId> enemyId;
	std::optional<EnemyPursuitStopReason> enemyPursuitStopReason;
	std::optional<int> enemyPursuitStepsCommitted;
	std::optional<EnemyAttackTransition> enemyAttackTransition;
};

} // namespace dev
