#pragma once

#include <optional>

#include "actions/ActionResult.hpp"
#include "commands/MovementCommand.hpp"
#include "interaction/DestinationAction.hpp"
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
};

struct MovementEvent {
	MovementEventType type;
	PlayerId playerId = 0;
	Point tile;
	std::optional<MovementCommandType> commandType;
	std::optional<DestinationActionType> actionType;
	std::optional<ActionResultType> actionResult;
};

} // namespace dev

