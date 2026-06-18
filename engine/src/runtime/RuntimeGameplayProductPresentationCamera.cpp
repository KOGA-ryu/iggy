#include "runtime/RuntimeGameplayProductPresentationCamera.hpp"

namespace iggy::runtime {
namespace {

LevelRenderFrame2DConfig LevelConfig(
	const RuntimeGameplayProductPresentationCameraConfig &config)
{
	LevelRenderFrame2DConfig levelConfig;
	levelConfig.cameraView = config.cameraView;
	levelConfig.includeNpcCommands = config.includeNpcCommands;
	levelConfig.useTileChunkCache = config.useTileChunkCache;
	levelConfig.tileChunkCache = config.tileChunkCache;
	return levelConfig;
}

void ApplyFallbackCamera(
	RuntimeGameplayProductPresentationCameraResult &result,
	const RuntimeGameplayProductPresentationCameraConfig &config)
{
	if (config.hasPreviousCamera) {
		result.presentationCamera = config.previousCamera;
		result.usedPrevious = true;
	} else {
		result.presentationCamera = config.fallbackCamera;
		result.usedFallback = true;
	}
}

} // namespace

RuntimeGameplayProductPresentationCameraResult
RuntimeGameplayProductPresentationCamera::build(
	const RuntimeGameplayProductPlayModeState &state,
	const RuntimeGameplayProductPresentationCameraConfig &config) const
{
	RuntimeGameplayProductPresentationCameraResult result;
	result.levelRenderConfig = LevelConfig(config);

	if (!state.loop.loaded) {
		ApplyFallbackCamera(result, config);
		result.status = RuntimeGameplayProductPresentationCameraStatus::NotLoaded;
		return result;
	}

	if (!state.loop.currentState.session.hasPlayer || !config.followPlayer) {
		ApplyFallbackCamera(result, config);
		result.status = result.usedPrevious
			? RuntimeGameplayProductPresentationCameraStatus::UsedPrevious
			: RuntimeGameplayProductPresentationCameraStatus::UsedFallback;
		return result;
	}

	result.followedPlayer = true;
	const Vec2 playerPosition = state.loop.currentState.session.player.position;
	if (!config.hasPreviousCamera) {
		const CameraRigResult rig = CameraRig {}.update(
			{ playerPosition },
			playerPosition,
			config.rig);
		result.presentationCamera = rig.state;
		result.clamped = rig.clamped;
		result.status =
			RuntimeGameplayProductPresentationCameraStatus::Initialized;
		return result;
	}

	const CameraRigResult rig = CameraRig {}.update(
		config.previousCamera,
		playerPosition,
		config.rig);
	result.presentationCamera = rig.state;
	result.usedPrevious = true;
	result.clamped = rig.clamped;
	result.status =
		RuntimeGameplayProductPresentationCameraStatus::FollowedPlayer;
	return result;
}

} // namespace iggy::runtime
