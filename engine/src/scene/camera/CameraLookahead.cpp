#include "scene/camera/CameraLookahead.hpp"

#include <algorithm>

namespace iggy {

CameraLookaheadResult CameraLookahead::apply(Vec2 targetPosition, Vec2 targetVelocity, const CameraLookaheadConfig &config) const
{
	if (config.distance <= 0.0F)
		return { targetPosition, false };

	const float effectiveMinSpeed = std::max(0.0F, config.minSpeed);
	const float speed = targetVelocity.length();
	if (speed == 0.0F || speed < effectiveMinSpeed)
		return { targetPosition, false };

	return { targetPosition + targetVelocity.normalized() * config.distance, true };
}

} // namespace iggy
