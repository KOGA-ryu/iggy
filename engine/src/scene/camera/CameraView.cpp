#include "scene/camera/CameraView.hpp"

#include <cmath>

namespace iggy {

CameraViewResult CameraView::visibleWorldBounds(CameraState state, const CameraViewConfig &config) const
{
	const Vec2 effectiveViewport {
		std::fabs(config.viewportSize.x),
		std::fabs(config.viewportSize.y),
	};
	const float effectiveZoom = config.zoom > 0.0F ? config.zoom : 1.0F;
	const Vec2 halfExtent = effectiveViewport / (2.0F * effectiveZoom);

	return {
		{ state.position - halfExtent, state.position + halfExtent },
		halfExtent,
		effectiveZoom,
	};
}

} // namespace iggy
