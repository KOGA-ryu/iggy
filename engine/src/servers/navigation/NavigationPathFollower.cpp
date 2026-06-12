#include "servers/navigation/NavigationPathFollower.hpp"

namespace iggy::navigation {

NavigationPathFollowResult NavigationPathFollower::step(const NavigationPath &path, NavigationPathFollowState state, Vec2 currentPosition, float maxDistance) const
{
	NavigationPathFollowResult result { currentPosition, state.waypointIndex, false, state.completed };
	if (!path.found() || path.waypoints.empty() || state.completed) {
		result.completed = true;
		return result;
	}

	if (result.waypointIndex >= path.waypoints.size())
		result.waypointIndex = path.waypoints.size() - 1;

	float remainingDistance = maxDistance;
	Vec2 position = currentPosition;
	while (remainingDistance >= 0.0F && result.waypointIndex < path.waypoints.size()) {
		const Vec2 target = path.waypoints[result.waypointIndex];
		const Vec2 segment = target - position;
		const float distanceToTarget = segment.length();

		if (distanceToTarget == 0.0F) {
			if (result.waypointIndex + 1 >= path.waypoints.size()) {
				result.completed = true;
				break;
			}
			++result.waypointIndex;
			continue;
		}

		if (remainingDistance < distanceToTarget) {
			position = position + segment.normalized() * remainingDistance;
			result.moved = result.moved || remainingDistance > 0.0F;
			break;
		}

		position = target;
		result.moved = true;
		remainingDistance -= distanceToTarget;
		if (result.waypointIndex + 1 >= path.waypoints.size()) {
			result.completed = true;
			break;
		}
		++result.waypointIndex;
	}

	result.position = position;
	return result;
}

} // namespace iggy::navigation
