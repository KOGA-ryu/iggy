#include "scene/ai/NpcAiNavigationRequest2D.hpp"

#include "servers/navigation/NavigationGridValidator.hpp"

namespace iggy {

bool NpcAiNavigationRequest2DResult::hasNavigationRequest() const
{
	return status == NpcAiNavigationRequest2DStatus::Built;
}

NpcAiNavigationRequest2DResult NpcAiNavigationRequestBuilder2D::build(
	const NpcAiRouteRequest2DResult &route,
	const LevelTileMap &map) const
{
	NpcAiNavigationRequest2DResult result;
	result.route = route;

	if (!route.hasRouteRequest()) {
		result.status = NpcAiNavigationRequest2DStatus::NoRouteRequest;
		return result;
	}

	const navigation::NavigationGridValidationInput input {
		true,
		true,
		route.targetPosition,
	};
	result.request = navigation::NavigationGridValidator {}.validate(map, input);
	result.status = result.request.accepted()
		? NpcAiNavigationRequest2DStatus::Built
		: NpcAiNavigationRequest2DStatus::NavigationRequestInvalid;
	return result;
}

} // namespace iggy
