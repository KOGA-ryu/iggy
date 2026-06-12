#pragma once

#include "core/math/Aabb2.hpp"
#include "core/math/Vec2.hpp"
#include "scene/camera/CameraState.hpp"

namespace iggy {

struct CameraViewConfig {
	Vec2 viewportSize;
	float zoom = 1.0F;
};

struct CameraViewResult {
	Aabb2 bounds;
	Vec2 halfExtent;
	float effectiveZoom = 1.0F;
};

class CameraView {
public:
	[[nodiscard]] CameraViewResult visibleWorldBounds(CameraState state, const CameraViewConfig &config) const;
};

} // namespace iggy
