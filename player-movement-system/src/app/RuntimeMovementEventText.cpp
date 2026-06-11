#include "RuntimeMovementEventText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(MovementEventType type)
{
	switch (type) {
	case MovementEventType::CommandAccepted:
		return "CommandAccepted";
	case MovementEventType::CommandRejected:
		return "CommandRejected";
	case MovementEventType::PathStarted:
		return "PathStarted";
	case MovementEventType::PathBlocked:
		return "PathBlocked";
	case MovementEventType::StepCommitted:
		return "StepCommitted";
	case MovementEventType::DestinationActionReady:
		return "DestinationActionReady";
	case MovementEventType::ActionExecuted:
		return "ActionExecuted";
	case MovementEventType::ActionRejected:
		return "ActionRejected";
	case MovementEventType::AnimationLocked:
		return "AnimationLocked";
	case MovementEventType::AnimationUnlocked:
		return "AnimationUnlocked";
	case MovementEventType::EnemyPursuitStopped:
		return "EnemyPursuitStopped";
	case MovementEventType::EnemyAttackTransitioned:
		return "EnemyAttackTransitioned";
	}
	return "Unknown";
}

const char *ToString(EnemyAttackTransition transition)
{
	switch (transition) {
	case EnemyAttackTransition::None:
		return "None";
	case EnemyAttackTransition::WindupStarted:
		return "WindupStarted";
	case EnemyAttackTransition::WindupCompleted:
		return "WindupCompleted";
	case EnemyAttackTransition::RecoveryCompleted:
		return "RecoveryCompleted";
	case EnemyAttackTransition::RecoveryCompletedAndWindupStarted:
		return "RecoveryCompletedAndWindupStarted";
	}
	return "Unknown";
}

const char *ToString(EnemyPursuitStopReason reason)
{
	switch (reason) {
	case EnemyPursuitStopReason::BudgetSpent:
		return "BudgetSpent";
	case EnemyPursuitStopReason::Blocked:
		return "Blocked";
	case EnemyPursuitStopReason::AlreadyAtTarget:
		return "AlreadyAtTarget";
	case EnemyPursuitStopReason::AttackRangeReached:
		return "AttackRangeReached";
	}
	return "Unknown";
}

const char *ToString(MovementCommandType type)
{
	switch (type) {
	case MovementCommandType::WalkTo:
		return "WalkTo";
	case MovementCommandType::MoveThenAct:
		return "MoveThenAct";
	case MovementCommandType::StandAndAct:
		return "StandAndAct";
	case MovementCommandType::Stop:
		return "Stop";
	}
	return "Unknown";
}

std::string PointText(Point point)
{
	std::ostringstream line;
	line << "(" << point.x << "," << point.y << ")";
	return line.str();
}

} // namespace

std::string RuntimeMovementEventText::formatEvent(std::string_view label, const MovementEvent &event) const
{
	std::ostringstream line;
	line << label << " type=" << ToString(event.type)
	     << " player=" << static_cast<int>(event.playerId)
	     << " tile=" << PointText(event.tile);
	if (event.commandType.has_value())
		line << " command=" << ToString(*event.commandType);
	if (event.enemyId.has_value())
		line << " enemy=" << *event.enemyId;
	if (event.enemyPursuitStopReason.has_value())
		line << " pursuitStop=" << ToString(*event.enemyPursuitStopReason);
	if (event.enemyPursuitStepsCommitted.has_value())
		line << " pursuitSteps=" << *event.enemyPursuitStepsCommitted;
	if (event.enemyAttackTransition.has_value())
		line << " attackTransition=" << ToString(*event.enemyAttackTransition);
	return line.str();
}

} // namespace dev
