#include "scene/camera/CameraRig.hpp"

namespace iggy {

CameraRigResult CameraRig::update(CameraState current, Vec2 targetPosition, const CameraRigConfig &config) const
{
	const CameraFollowResult followed = CameraFollow {}.step(current, targetPosition, config.follow);
	if (!config.clampToBounds)
		return { followed.state, followed.moved, false };

	const CameraClampResult clamped = CameraClamp {}.clamp(followed.state, config.bounds);
	return { clamped.state, followed.moved, clamped.clamped };
}

} // namespace iggy
