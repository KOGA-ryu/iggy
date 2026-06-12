#include "scene/camera/CameraFollow.hpp"

#include <algorithm>

namespace iggy {

CameraFollowResult CameraFollow::step(CameraState state, Vec2 targetPosition, const CameraFollowConfig &config) const
{
	CameraFollowResult result;
	result.state = state;

	const float deadZoneRadius = std::max(0.0F, config.deadZoneRadius);
	if (config.maxStep <= 0.0F)
		return result;

	const Vec2 offset = targetPosition - state.position;
	const float distance = offset.length();
	if (distance <= deadZoneRadius)
		return result;

	const float travelDistance = std::min(config.maxStep, distance - deadZoneRadius);
	const Vec2 nextPosition = state.position + offset.normalized() * travelDistance;
	result.moved = !(nextPosition == state.position);
	result.state.position = nextPosition;
	return result;
}

} // namespace iggy
