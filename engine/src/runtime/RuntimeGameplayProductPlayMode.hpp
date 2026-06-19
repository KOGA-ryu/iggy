#pragma once

#include "runtime/RuntimeGameplayProductLoop.hpp"
#include "runtime/RuntimeGameplayProductPlaySurfaceFrame.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductPlayModeBuildStatus {
	Ready,
	LoadFailed,
};

struct RuntimeGameplayProductPlayModeState {
	RuntimeGameplayProductLoopState loop;
	bool hasInputFocus = true;
};

struct RuntimeGameplayProductPlayModeBuildResult {
	RuntimeGameplayProductPlayModeBuildStatus status =
		RuntimeGameplayProductPlayModeBuildStatus::LoadFailed;
	RuntimeGameplayProductPlayModeState state;
	RuntimeGameplayProductLoopBuildResult loop;
};

enum class RuntimeGameplayProductPlayModeFrameStatus {
	Stepped,
	NotLoaded,
	NoFrameAvailable,
};

struct RuntimeGameplayProductPlayModeFrameInput {
	RuntimeGameplayProductPlayModeState state;
	RuntimeGameplayProductInputFrame2D inputFrame;
	bool allowFreePlayFrameWhenNoFrameAvailable = false;
	CameraState presentationCamera;
	LevelRenderFrame2DConfig levelRenderConfig;
};

struct RuntimeGameplayProductPlayModeFrameResult {
	RuntimeGameplayProductPlayModeFrameStatus status =
		RuntimeGameplayProductPlayModeFrameStatus::NotLoaded;
	RuntimeGameplayProductPlayModeState state;
	RuntimeGameplayProductPlaySurfaceFrameResult surface;
};

class RuntimeGameplayProductPlayMode {
public:
	[[nodiscard]] RuntimeGameplayProductPlayModeBuildResult build(
		const RuntimeGameplayProductLoopBuildResult &loop) const;

	[[nodiscard]] RuntimeGameplayProductPlayModeState withInputFocus(
		RuntimeGameplayProductPlayModeState state,
		bool hasInputFocus) const;

	[[nodiscard]] RuntimeGameplayProductPlayModeFrameResult frame(
		const RuntimeGameplayProductPlayModeFrameInput &input) const;
};

} // namespace iggy::runtime
