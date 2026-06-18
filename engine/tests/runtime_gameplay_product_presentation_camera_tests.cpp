#include "runtime/RuntimeGameplayProductPresentationCamera.hpp"

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

using Status =
	iggy::runtime::RuntimeGameplayProductPresentationCameraStatus;

const iggy::ResourceId PlayerId { "player:presentation-camera" };

iggy::runtime::RuntimeGameplayProductPlayModeState PlayState(
	bool loaded,
	bool hasPlayer,
	iggy::Vec2 playerPosition = { 4.0F, 0.0F })
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state;
	state.loop.loaded = loaded;
	state.loop.currentState.session.level.map = MapFromRows({ "...." });
	state.loop.currentState.session.hasPlayer = hasPlayer;
	if (hasPlayer) {
		state.loop.currentState.session.player = PlayerAgent(
			PlayerId,
			playerPosition,
			{ 0, 0 },
			iggy::PlayerMovementStatus::Idle,
			iggy::PlayerFacing2D::East);
	}
	state.loop.nextFrameIndex = 3;
	return state;
}

iggy::runtime::RuntimeGameplayProductPresentationCameraConfig Config()
{
	iggy::runtime::RuntimeGameplayProductPresentationCameraConfig config;
	config.fallbackCamera = { { -2.0F, 5.0F } };
	config.cameraView = { { 8.0F, 6.0F }, 2.0F };
	config.includeNpcCommands = false;
	config.useTileChunkCache = false;
	return config;
}

iggy::runtime::RuntimeGameplayProductPresentationCameraResult Build(
	const iggy::runtime::RuntimeGameplayProductPlayModeState &state,
	const iggy::runtime::RuntimeGameplayProductPresentationCameraConfig &config)
{
	return iggy::runtime::RuntimeGameplayProductPresentationCamera {}.build(
		state,
		config);
}

bool SameCamera(iggy::CameraState actual, iggy::CameraState expected)
{
	return NearVec(actual.position, expected.position);
}

void ExpectRenderConfigForwarded(
	const iggy::LevelRenderFrame2DConfig &actual,
	const iggy::runtime::RuntimeGameplayProductPresentationCameraConfig &expected,
	const char *message)
{
	Expect(NearVec(actual.cameraView.viewportSize, expected.cameraView.viewportSize),
		std::string(message) + " should forward camera view viewport");
	Expect(actual.cameraView.zoom == expected.cameraView.zoom,
		std::string(message) + " should forward camera view zoom");
	Expect(actual.includeNpcCommands == expected.includeNpcCommands,
		std::string(message) + " should forward NPC command inclusion");
	Expect(actual.useTileChunkCache == expected.useTileChunkCache,
		std::string(message) + " should forward tile chunk cache flag");
	Expect(actual.tileChunkCache == expected.tileChunkCache,
		std::string(message) + " should forward tile chunk cache pointer");
}

void TestNotLoadedUsesFallbackCameraAndConfig()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeState state =
		PlayState(false, true);
	const auto config = Config();

	const auto result = Build(state, config);

	Expect(result.status == Status::NotLoaded,
		"not-loaded state should report not loaded");
	Expect(SameCamera(result.presentationCamera, config.fallbackCamera),
		"not-loaded state should use fallback camera without previous camera");
	Expect(result.usedFallback, "not-loaded state should report fallback use");
	Expect(!result.usedPrevious, "not-loaded state should not report previous use");
	Expect(!result.followedPlayer, "not-loaded state should not follow player");
	ExpectRenderConfigForwarded(result.levelRenderConfig, config,
		"not-loaded result");
}

void TestNotLoadedUsesPreviousCameraWhenAvailable()
{
	auto config = Config();
	config.hasPreviousCamera = true;
	config.previousCamera = { { 7.0F, 9.0F } };

	const auto result = Build(PlayState(false, true), config);

	Expect(result.status == Status::NotLoaded,
		"not-loaded previous-camera state should still report not loaded");
	Expect(SameCamera(result.presentationCamera, config.previousCamera),
		"not-loaded state should prefer previous camera when supplied");
	Expect(result.usedPrevious, "not-loaded state should report previous use");
	Expect(!result.usedFallback, "not-loaded state should not report fallback use");
}

void TestLoadedPlayerWithoutPreviousInitializesAtPlayer()
{
	const iggy::Vec2 playerPosition { 4.0F, 1.0F };
	auto config = Config();
	config.rig.follow = { 1.0F, 0.0F };

	const auto result = Build(PlayState(true, true, playerPosition), config);

	Expect(result.status == Status::Initialized,
		"loaded player state without previous camera should initialize camera");
	Expect(SameCamera(result.presentationCamera, { playerPosition }),
		"initialized camera should start at player position");
	Expect(result.followedPlayer,
		"initialized loaded player state should report player follow target");
	Expect(!result.usedPrevious && !result.usedFallback,
		"initialized camera should not report previous or fallback use");
}

void TestLoadedPreviousCameraFollowsPlayerThroughRig()
{
	auto config = Config();
	config.hasPreviousCamera = true;
	config.previousCamera = { { 0.0F, 0.0F } };
	config.rig.follow = { 1.5F, 0.0F };

	const auto result = Build(PlayState(true, true, { 4.0F, 0.0F }), config);

	Expect(result.status == Status::FollowedPlayer,
		"loaded previous camera should report followed player");
	Expect(result.usedPrevious, "follow should start from previous camera");
	Expect(result.followedPlayer, "follow should report player target");
	Expect(SameCamera(result.presentationCamera, { { 1.5F, 0.0F } }),
		"followed camera should match CameraRig movement");
}

void TestFollowDisabledPreservesPreviousCamera()
{
	auto config = Config();
	config.followPlayer = false;
	config.hasPreviousCamera = true;
	config.previousCamera = { { 2.0F, 3.0F } };
	config.rig.follow = { 10.0F, 0.0F };

	const auto result = Build(PlayState(true, true, { 20.0F, 20.0F }), config);

	Expect(result.status == Status::UsedPrevious,
		"follow-disabled state with previous camera should report previous use");
	Expect(SameCamera(result.presentationCamera, config.previousCamera),
		"follow-disabled state should preserve previous camera");
	Expect(result.usedPrevious, "follow-disabled state should flag previous use");
	Expect(!result.followedPlayer,
		"follow-disabled state should not report player following");
}

void TestClampResultComesFromCameraRig()
{
	auto config = Config();
	config.hasPreviousCamera = true;
	config.previousCamera = { { 10.0F, 0.0F } };
	config.rig.follow = { 0.0F, 0.0F };
	config.rig.clampToBounds = true;
	config.rig.bounds = { { -5.0F, -1.0F }, { 5.0F, 1.0F } };

	const auto result = Build(PlayState(true, true, { 10.0F, 0.0F }), config);

	Expect(result.status == Status::FollowedPlayer,
		"clamped loaded state should still report followed player policy");
	Expect(result.clamped, "camera policy should expose CameraRig clamp result");
	Expect(SameCamera(result.presentationCamera, { { 5.0F, 0.0F } }),
		"camera policy should return clamped camera position");
}

void TestMissingPlayerUsesPreviousOrFallback()
{
	auto previousConfig = Config();
	previousConfig.hasPreviousCamera = true;
	previousConfig.previousCamera = { { 3.0F, 4.0F } };
	const auto previous = Build(PlayState(true, false), previousConfig);

	Expect(previous.status == Status::UsedPrevious,
		"loaded state without player should use previous camera when supplied");
	Expect(SameCamera(previous.presentationCamera, previousConfig.previousCamera),
		"missing-player previous path should preserve previous camera");
	Expect(!previous.followedPlayer,
		"missing-player previous path should not report player following");

	const auto fallbackConfig = Config();
	const auto fallback = Build(PlayState(true, false), fallbackConfig);

	Expect(fallback.status == Status::UsedFallback,
		"loaded state without player should use fallback camera without previous");
	Expect(SameCamera(fallback.presentationCamera, fallbackConfig.fallbackCamera),
		"missing-player fallback path should preserve fallback camera");
	Expect(fallback.usedFallback,
		"missing-player fallback path should report fallback use");
}

void TestLevelRenderConfigForwardsCacheFields()
{
	iggy::LevelTileRenderChunkCache cache;
	cache.chunks.push_back({});

	auto config = Config();
	config.cameraView = { { 12.0F, 9.0F }, 3.0F };
	config.includeNpcCommands = true;
	config.useTileChunkCache = true;
	config.tileChunkCache = &cache;
	const std::size_t cacheChunkCount = cache.chunks.size();

	const auto result = Build(PlayState(true, true), config);

	ExpectRenderConfigForwarded(result.levelRenderConfig, config,
		"loaded result");
	Expect(cache.chunks.size() == cacheChunkCount,
		"camera policy should not mutate tile chunk cache target");
}

void TestInputsAreNotMutated()
{
	iggy::LevelTileRenderChunkCache cache;
	cache.chunks.push_back({});

	iggy::runtime::RuntimeGameplayProductPlayModeState state =
		PlayState(true, true, { 4.0F, 0.0F });
	auto config = Config();
	config.hasPreviousCamera = true;
	config.previousCamera = { { 0.0F, 0.0F } };
	config.rig.follow = { 2.0F, 0.0F };
	config.useTileChunkCache = true;
	config.tileChunkCache = &cache;

	const iggy::runtime::RuntimeGameplayProductPlayModeState stateBefore =
		state;
	const auto configBefore = config;
	const std::size_t cacheChunkCount = cache.chunks.size();

	(void)Build(state, config);

	Expect(state.loop.loaded == stateBefore.loop.loaded,
		"camera policy should not mutate loaded flag");
	Expect(state.hasInputFocus == stateBefore.hasInputFocus,
		"camera policy should not mutate play input focus");
	Expect(state.loop.nextFrameIndex == stateBefore.loop.nextFrameIndex,
		"camera policy should not mutate frame index");
	Expect(NearVec(
			   state.loop.currentState.session.player.position,
			   stateBefore.loop.currentState.session.player.position),
		"camera policy should not mutate player position");
	Expect(config.hasPreviousCamera == configBefore.hasPreviousCamera,
		"camera policy should not mutate config previous flag");
	Expect(SameCamera(config.previousCamera, configBefore.previousCamera),
		"camera policy should not mutate previous camera");
	Expect(SameCamera(config.fallbackCamera, configBefore.fallbackCamera),
		"camera policy should not mutate fallback camera");
	Expect(config.tileChunkCache == configBefore.tileChunkCache,
		"camera policy should not mutate cache pointer");
	Expect(cache.chunks.size() == cacheChunkCount,
		"camera policy should not mutate cache target");
}

} // namespace

int main()
{
	TestNotLoadedUsesFallbackCameraAndConfig();
	TestNotLoadedUsesPreviousCameraWhenAvailable();
	TestLoadedPlayerWithoutPreviousInitializesAtPlayer();
	TestLoadedPreviousCameraFollowsPlayerThroughRig();
	TestFollowDisabledPreservesPreviousCamera();
	TestClampResultComesFromCameraRig();
	TestMissingPlayerUsesPreviousOrFallback();
	TestLevelRenderConfigForwardsCacheFields();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
