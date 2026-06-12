#include "scene/camera/CameraClamp.hpp"

#include <algorithm>

namespace iggy {

CameraClampResult CameraClamp::clamp(CameraState state, Aabb2 bounds) const
{
	const Vec2 effectiveMin {
		std::min(bounds.min.x, bounds.max.x),
		std::min(bounds.min.y, bounds.max.y),
	};
	const Vec2 effectiveMax {
		std::max(bounds.min.x, bounds.max.x),
		std::max(bounds.min.y, bounds.max.y),
	};

	const Vec2 clampedPosition {
		std::max(effectiveMin.x, std::min(state.position.x, effectiveMax.x)),
		std::max(effectiveMin.y, std::min(state.position.y, effectiveMax.y)),
	};

	return { { clampedPosition }, !(clampedPosition == state.position) };
}

} // namespace iggy
