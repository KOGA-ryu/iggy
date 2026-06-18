#pragma once

#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "scene/camera/CameraRig.hpp"
#include "scene/camera/CameraState.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelRenderFrame2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductPresentationCameraStatus {
	NotLoaded,
	Initialized,
	UsedPrevious,
	FollowedPlayer,
	UsedFallback,
};

struct RuntimeGameplayProductPresentationCameraConfig {
	bool hasPreviousCamera = false;
	CameraState previousCamera;
	CameraState fallbackCamera;
	bool followPlayer = true;
	CameraRigConfig rig;
	CameraViewConfig cameraView;
	bool includeNpcCommands = true;
	bool useTileChunkCache = false;
	const LevelTileRenderChunkCache *tileChunkCache = nullptr;
};

struct RuntimeGameplayProductPresentationCameraResult {
	RuntimeGameplayProductPresentationCameraStatus status =
		RuntimeGameplayProductPresentationCameraStatus::NotLoaded;
	CameraState presentationCamera;
	LevelRenderFrame2DConfig levelRenderConfig;
	bool usedPrevious = false;
	bool followedPlayer = false;
	bool clamped = false;
	bool usedFallback = false;
};

class RuntimeGameplayProductPresentationCamera {
public:
	[[nodiscard]] RuntimeGameplayProductPresentationCameraResult build(
		const RuntimeGameplayProductPlayModeState &state,
		const RuntimeGameplayProductPresentationCameraConfig &config) const;
};

} // namespace iggy::runtime
