#include "runtime/RuntimeGameplayProductPointerProjection.hpp"

#include <cmath>

namespace {

float AxisWorldPoint(float min, float span, float viewportPoint, float viewportSize)
{
	if (viewportSize == 0.0F)
		return min;
	return min + (span * (viewportPoint / viewportSize));
}

} // namespace

namespace iggy::runtime {

RuntimeGameplayProductPointerProjectionResult
RuntimeGameplayProductPointerProjection::project(
	const RuntimeGameplayProductPointerProjectionInput &input) const
{
	RuntimeGameplayProductPointerProjectionResult result;

	CameraViewConfig viewConfig = input.cameraView;
	viewConfig.viewportSize = input.viewportSize;
	result.cameraView = CameraView {}.visibleWorldBounds(input.camera, viewConfig);

	const Vec2 effectiveViewport {
		std::fabs(input.viewportSize.x),
		std::fabs(input.viewportSize.y),
	};
	result.degenerateViewportX = effectiveViewport.x == 0.0F;
	result.degenerateViewportY = effectiveViewport.y == 0.0F;

	result.worldPoint = {
		AxisWorldPoint(
			result.cameraView.bounds.min.x,
			result.cameraView.halfExtent.x * 2.0F,
			input.viewportPoint.x,
			effectiveViewport.x),
		AxisWorldPoint(
			result.cameraView.bounds.min.y,
			result.cameraView.halfExtent.y * 2.0F,
			input.viewportPoint.y,
			effectiveViewport.y),
	};
	result.tile = tileForPoint(result.worldPoint);
	return result;
}

} // namespace iggy::runtime
