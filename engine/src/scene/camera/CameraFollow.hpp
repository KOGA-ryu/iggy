#pragma once

#include "core/math/Vec2.hpp"
#include "scene/camera/CameraState.hpp"

namespace iggy {

struct CameraFollowConfig {
	float maxStep = 0.0F;
	float deadZoneRadius = 0.0F;
};

struct CameraFollowResult {
	CameraState state;
	bool moved = false;
};

class CameraFollow {
public:
	[[nodiscard]] CameraFollowResult step(CameraState state, Vec2 targetPosition, const CameraFollowConfig &config) const;
};

} // namespace iggy
