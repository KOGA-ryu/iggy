#include "scene/npc/NpcActorEscapeRouteTarget2D.hpp"

namespace iggy {

bool NpcActorEscapeRouteTarget2D::ready() const
{
	return status == NpcActorEscapeRouteTarget2DStatus::Ready && requestsRoute;
}

NpcActorEscapeRouteTarget2D NpcActorEscapeRouteTargetProjector2D::project(
	const NpcActorMovementIntent2D &intent,
	const LevelTileMap &map,
	const NpcActorEscapeRouteTarget2DConfig &config) const
{
	NpcActorEscapeRouteTarget2D result;
	result.intent = intent;

	if (!intent.ready() || !intent.requestsMovement) {
		result.status = NpcActorEscapeRouteTarget2DStatus::NoMovementIntent;
		return result;
	}

	if (intent.type != NpcActorMovementIntent2DType::MoveAwayFrom) {
		result.status = NpcActorEscapeRouteTarget2DStatus::NotMoveAwayFrom;
		return result;
	}

	result.escape = NpcActorEscapeTargetProjector2D {}.project(intent, map, config.escape);
	if (!result.escape.ready()) {
		result.status = NpcActorEscapeRouteTarget2DStatus::EscapeTargetFailed;
		return result;
	}

	NpcActorRouteTarget2DConfig routeConfig = config.route;
	routeConfig.hasEscapeDestination = true;
	routeConfig.escapeDestination = result.escape.escapePosition;
	result.route = NpcActorRouteTargetProjector2D {}.project(intent, routeConfig);
	if (!result.route.ready()) {
		result.status = NpcActorEscapeRouteTarget2DStatus::RouteTargetFailed;
		return result;
	}

	result.status = NpcActorEscapeRouteTarget2DStatus::Ready;
	result.requestsRoute = true;
	return result;
}

} // namespace iggy
