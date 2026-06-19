#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayProductInputAdapter.hpp"
#include "runtime/RuntimeGameplayProductLoop.hpp"
#include "runtime/RuntimeGameplayProductPresentationFrame.hpp"
#include "scene/camera/CameraState.hpp"
#include "scene/level/LevelRenderFrame2D.hpp"
#include "scene/player/PlayerInputBinding2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductPlaySurfaceFrameStatus {
	Stepped,
	NotLoaded,
	NoFrameAvailable,
};

struct RuntimeGameplayProductPlaySurfaceFrameInput {
	RuntimeGameplayProductLoopState state;
	RuntimeGameplayProductInputFrame2D inputFrame;
	bool hasInputFocus = true;
	bool allowFreePlayFrameWhenNoFrameAvailable = false;
	CameraState presentationCamera;
	LevelRenderFrame2DConfig levelRenderConfig;
};

struct RuntimeGameplayProductPlaySurfaceFrameResult {
	RuntimeGameplayProductPlaySurfaceFrameStatus status =
		RuntimeGameplayProductPlaySurfaceFrameStatus::NotLoaded;
	RuntimeGameplayProductInputAdapterResult inputAdapter;
	PlayerInputBinding2DResult playerBinding;
	RuntimeGameplayProductLoopStepResult step;
	RuntimeGameplayProductPresentationFrameResult presentation;
	std::size_t ignoredInputEventCount = 0;
};

class RuntimeGameplayProductPlaySurfaceFrame {
public:
	[[nodiscard]] RuntimeGameplayProductPlaySurfaceFrameResult build(
		const RuntimeGameplayProductPlaySurfaceFrameInput &input) const;
};

} // namespace iggy::runtime
