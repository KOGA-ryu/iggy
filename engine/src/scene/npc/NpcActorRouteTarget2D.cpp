#include "scene/npc/NpcActorRouteTarget2D.hpp"

namespace {

bool WithinArrivalTolerance(iggy::Vec2 start, iggy::Vec2 target, float tolerance)
{
	return (target - start).length() <= tolerance;
}

void PreserveIntentFacts(iggy::NpcActorRouteTarget2D &target, const iggy::NpcActorMovementIntent2D &intent)
{
	target.intent = intent;
	target.npcId = intent.npcId;
	target.startPosition = intent.startPosition;
	target.moveMode = intent.moveMode;
	target.speedMultiplier = intent.speedMultiplier;
}

} // namespace

namespace iggy {

bool NpcActorRouteTarget2D::ready() const
{
	return status == NpcActorRouteTarget2DStatus::Ready && requestsRoute;
}

NpcActorRouteTarget2D NpcActorRouteTargetProjector2D::project(
	const NpcActorMovementIntent2D &intent,
	const NpcActorRouteTarget2DConfig &config) const
{
	NpcActorRouteTarget2D target;
	PreserveIntentFacts(target, intent);

	if (!intent.ready() || !intent.requestsMovement) {
		target.status = NpcActorRouteTarget2DStatus::NoMovementIntent;
		return target;
	}

	switch (intent.type) {
	case NpcActorMovementIntent2DType::MoveTo:
		target.type = NpcActorRouteTarget2DType::MoveTo;
		target.targetPosition = intent.targetPosition;
		if (WithinArrivalTolerance(intent.startPosition, intent.targetPosition, config.arrivalTolerance)) {
			target.status = NpcActorRouteTarget2DStatus::AlreadyAtTarget;
			return target;
		}

		target.status = NpcActorRouteTarget2DStatus::Ready;
		target.requestsRoute = true;
		return target;

	case NpcActorMovementIntent2DType::MoveAwayFrom:
		target.sourcePosition = intent.targetPosition;
		if (!config.hasEscapeDestination) {
			target.status = NpcActorRouteTarget2DStatus::NeedsEscapeDestination;
			return target;
		}

		target.type = NpcActorRouteTarget2DType::MoveAwayFrom;
		target.targetPosition = config.escapeDestination;
		if (WithinArrivalTolerance(intent.startPosition, config.escapeDestination, config.arrivalTolerance)) {
			target.status = NpcActorRouteTarget2DStatus::AlreadyAtTarget;
			return target;
		}

		target.status = NpcActorRouteTarget2DStatus::Ready;
		target.requestsRoute = true;
		return target;

	case NpcActorMovementIntent2DType::None:
		target.status = NpcActorRouteTarget2DStatus::InvalidTarget;
		return target;
	}

	target.status = NpcActorRouteTarget2DStatus::InvalidTarget;
	return target;
}

} // namespace iggy
