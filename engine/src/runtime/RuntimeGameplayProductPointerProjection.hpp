#pragma once

#include "core/math/Vec2.hpp"
#include "scene/camera/CameraState.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductPointerProjectionStatus {
	Projected,
};

struct RuntimeGameplayProductPointerProjectionInput {
	Vec2 viewportPoint;
	Vec2 viewportSize;
	CameraState camera;
	CameraViewConfig cameraView;
};

struct RuntimeGameplayProductPointerProjectionResult {
	RuntimeGameplayProductPointerProjectionStatus status =
		RuntimeGameplayProductPointerProjectionStatus::Projected;
	Vec2 worldPoint;
	TileCoord tile;
	CameraViewResult cameraView;
	bool degenerateViewportX = false;
	bool degenerateViewportY = false;
};

class RuntimeGameplayProductPointerProjection {
public:
	[[nodiscard]] RuntimeGameplayProductPointerProjectionResult project(
		const RuntimeGameplayProductPointerProjectionInput &input) const;
};

} // namespace iggy::runtime
