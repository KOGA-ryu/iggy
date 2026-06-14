#include "scene/npc/NpcActorPathStep2D.hpp"

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

bool Near(iggy::Vec2 left, iggy::Vec2 right, float tolerance)
{
	return (right - left).length() <= tolerance;
}

void PreservePathFacts(iggy::NpcActorPathStep2D &step, const iggy::NpcActorPathReport2D &pathReport)
{
	step.pathReport = pathReport;
	step.npcId = pathReport.navigation.route.npcId;
	step.oldPosition = pathReport.navigation.route.startPosition;
	step.proposedPosition = step.oldPosition;
	step.oldTile = iggy::tileForPoint(step.oldPosition);
	step.proposedTile = step.oldTile;
	step.moveMode = pathReport.navigation.route.moveMode;
}

} // namespace

namespace iggy {

bool NpcActorPathStep2D::proposed() const
{
	return status == NpcActorPathStep2DStatus::Proposed && requestsMovement;
}

bool NpcActorPathStep2D::ready() const
{
	return proposed();
}

NpcActorPathStep2D NpcActorPathStepper2D::step(
	const NpcActorPathReport2D &pathReport,
	const NpcActorPathStep2DConfig &config) const
{
	NpcActorPathStep2D result;
	PreservePathFacts(result, pathReport);

	if (!pathReport.hasPath() || !pathReport.requestsStep) {
		result.status = NpcActorPathStep2DStatus::NoPath;
		return result;
	}

	if (!KnownMoveMode(result.moveMode)) {
		result.status = NpcActorPathStep2DStatus::InvalidMoveMode;
		return result;
	}

	if (Near(result.oldPosition, pathReport.navigation.route.targetPosition, config.arrivalTolerance)) {
		result.status = NpcActorPathStep2DStatus::AlreadyAtTarget;
		result.completedPath = true;
		result.follower = { result.oldPosition, 0, false, true };
		return result;
	}

	result.requestedDistance = config.baseStepDistance;
	result.maxDistance = config.baseStepDistance * npcMoveModeSpeedMultiplier(result.moveMode);
	if (result.maxDistance <= 0.0F || !npcMoveModeMoves(result.moveMode)) {
		result.status = NpcActorPathStep2DStatus::ZeroStep;
		return result;
	}

	result.follower = navigation::NavigationPathFollower {}.step(
		pathReport.path,
		{},
		result.oldPosition,
		result.maxDistance);
	result.proposedPosition = result.follower.position;
	result.proposedTile = tileForPoint(result.proposedPosition);
	result.completedPath = result.follower.completed;

	if (!result.follower.moved || Near(result.proposedPosition, result.oldPosition, config.arrivalTolerance)) {
		result.status = NpcActorPathStep2DStatus::AlreadyAtTarget;
		result.requestsMovement = false;
		return result;
	}

	result.status = NpcActorPathStep2DStatus::Proposed;
	result.requestsMovement = true;
	return result;
}

} // namespace iggy
