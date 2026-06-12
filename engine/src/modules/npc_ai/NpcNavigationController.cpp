#include "modules/npc_ai/NpcNavigationController.hpp"

#include "servers/navigation/NavigationGridPathfinder.hpp"
#include "servers/navigation/NavigationGridValidator.hpp"

namespace iggy::npc_ai {

namespace {

NpcNavigationResult Result(NpcNavigationStatus status, Vec2 position, navigation::NavigationRequestStatus requestStatus, navigation::NavigationPathStatus pathStatus, navigation::NavigationPathFollowState followState)
{
	return { status, position, requestStatus, pathStatus, followState };
}

} // namespace

NpcNavigationResult NpcNavigationController::step(const LevelTileMap &map, Vec2 currentPosition, const NpcMovementPlan &plan, navigation::NavigationPathFollowState followState, float maxDistance) const
{
	const navigation::NavigationRequest request = navigation::NavigationGridValidator {}.validate(map, plan);
	if (request.status == navigation::NavigationRequestStatus::None)
		return Result(NpcNavigationStatus::NoMovement, currentPosition, request.status, navigation::NavigationPathStatus::NoPath, followState);
	if (!request.accepted())
		return Result(NpcNavigationStatus::RequestRejected, currentPosition, request.status, navigation::NavigationPathStatus::DestinationRejected, followState);

	const navigation::NavigationPath path = navigation::NavigationGridPathfinder {}.findPath(map, currentPosition, request);
	if (!path.found())
		return Result(NpcNavigationStatus::PathNotFound, currentPosition, request.status, path.status, followState);

	const navigation::NavigationPathFollowResult followResult = navigation::NavigationPathFollower {}.step(path, followState, currentPosition, maxDistance);
	navigation::NavigationPathFollowState nextState { followResult.waypointIndex, followResult.completed };
	const NpcNavigationStatus status = followResult.completed ? NpcNavigationStatus::Arrived : NpcNavigationStatus::Moving;
	return Result(status, followResult.position, request.status, path.status, nextState);
}

} // namespace iggy::npc_ai
