#pragma once

#include "runtime/RuntimeGameplayProductLoop.hpp"
#include "scene/camera/CameraState.hpp"
#include "scene/level/LevelRenderFrame2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductPresentationFrameStatus {
	Rendered,
	NotLoaded,
};

struct RuntimeGameplayProductPresentationFrameInput {
	RuntimeGameplayProductLoopState state;
	CameraState presentationCamera;
	LevelRenderFrame2DConfig levelRenderConfig;
};

struct RuntimeGameplayProductPresentationFrameResult {
	RuntimeGameplayProductPresentationFrameStatus status =
		RuntimeGameplayProductPresentationFrameStatus::NotLoaded;
	CameraState presentationCamera;
	LevelRenderFrame2DResult levelFrame;
};

class RuntimeGameplayProductPresentationFrame {
public:
	[[nodiscard]] RuntimeGameplayProductPresentationFrameResult build(
		const RuntimeGameplayProductPresentationFrameInput &input) const;
};

} // namespace iggy::runtime
