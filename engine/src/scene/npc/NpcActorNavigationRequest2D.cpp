#include "scene/npc/NpcActorNavigationRequest2D.hpp"

#include "modules/npc_ai/NpcMovementPlan.hpp"
#include "servers/navigation/NavigationGridValidator.hpp"

namespace iggy {

bool NpcActorNavigationRequest2D::ready() const
{
	return status == NpcActorNavigationRequest2DStatus::Ready && requestsPath;
}

NpcActorNavigationRequest2D NpcActorNavigationRequestBuilder2D::build(
	const NpcActorRouteTarget2D &route,
	const LevelTileMap &map) const
{
	NpcActorNavigationRequest2D result;
	result.route = route;

	if (!route.ready() || !route.requestsRoute) {
		result.status = NpcActorNavigationRequest2DStatus::NoRouteTarget;
		return result;
	}

	const npc_ai::NpcMovementPlan plan {
		npc_ai::NpcMovementPlanType::MoveTo,
		route.targetPosition,
	};
	result.request = navigation::NavigationGridValidator {}.validate(map, plan);
	if (!result.request.accepted()) {
		result.status = NpcActorNavigationRequest2DStatus::NavigationRejected;
		return result;
	}

	result.status = NpcActorNavigationRequest2DStatus::Ready;
	result.requestsPath = true;
	return result;
}

} // namespace iggy
