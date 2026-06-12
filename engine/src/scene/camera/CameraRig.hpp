#pragma once

#include "core/math/Aabb2.hpp"
#include "core/math/Vec2.hpp"
#include "scene/camera/CameraClamp.hpp"
#include "scene/camera/CameraFollow.hpp"
#include "scene/camera/CameraState.hpp"

namespace iggy {

struct CameraRigConfig {
	CameraFollowConfig follow;
	bool clampToBounds = false;
	Aabb2 bounds;
};

struct CameraRigResult {
	CameraState state;
	bool followed = false;
	bool clamped = false;
};

class CameraRig {
public:
	[[nodiscard]] CameraRigResult update(CameraState current, Vec2 targetPosition, const CameraRigConfig &config) const;
};

} // namespace iggy
