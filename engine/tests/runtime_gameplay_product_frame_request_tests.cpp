#include "runtime/RuntimeGameplayProductFrameRequest.hpp"

#include <cstdlib>
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

using RequestStatus =
	iggy::runtime::RuntimeGameplayProductFrameRequestStatus;
using FrameStatus = iggy::runtime::RuntimeGameplayProductPlayModeFrameStatus;
using CameraStatus =
	iggy::runtime::RuntimeGameplayProductPresentationCameraStatus;
using PresentationStatus =
	iggy::runtime::RuntimeGameplayProductPresentationFrameStatus;
using LoopStatus = iggy::runtime::RuntimeGameplayProductLoopStatus;
using SurfaceStatus =
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameStatus;
using Control = iggy::runtime::RuntimeGameplayProductInputControl2D;

const iggy::ResourceId PlayerId { "player:frame-request" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };
const iggy::ResourceId NpcMaterial { "material:npc" };

iggy::LevelTileMap LevelMap(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = iggy::ResourceId { "level:frame-request" };
	return map;
}

iggy::runtime::RuntimeGameplayState GameplayState(
	const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = map;
	state.session.level.map.playerStart = { 0, 0 };
	state.session.hasPlayer = true;
	state.session.player = PlayerAgent(
		PlayerId,
		{ 0.5F, 0.5F },
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::East);
	return state;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig()
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = 10.0F;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::AiMap2D AiMap()
{
	const iggy::AiMap2DBuildResult result =
		iggy::AiMap2DBuilder {}.build({});
	Expect(result.built, "frame-request AI map should build");
	return result.map;
}

iggy::runtime::RuntimeGameplayFrameInput PlayerFrame(
	iggy::runtime::RuntimeGameplayState state)
{
	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = PlayerId;
	input.fallbackPlayerPosition = { 0.5F, 0.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	return input;
}

iggy::runtime::RuntimeGameplayScenarioFrame ScenarioFrame(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame;
	frame.playerFrame = PlayerFrame(state);
	frame.movementMap = map;
	frame.aiMap = AiMap();
	frame.refreshAiMap = AiMap();
	return { frame };
}

iggy::runtime::RuntimeGameplayProductPlayModeState ReadyState(
	std::size_t frameCount = 2)
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);

	iggy::runtime::RuntimeGameplayProductLoopBuildResult loop;
	loop.status = LoopStatus::Ready;
	loop.state.loaded = true;
	loop.state.sourcePath = "in-memory-frame-request";
	loop.state.initialState = state;
	loop.state.currentState = state;
	loop.state.scenario.initialState = state;
	for (std::size_t index = 0; index < frameCount; ++index)
		loop.state.scenario.frames.push_back(ScenarioFrame(state, map));

	return iggy::runtime::RuntimeGameplayProductPlayMode {}.build(loop).state;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D Event(Control control)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event;
	event.control = control;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D TileEvent(
	iggy::TileCoord tile)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event =
		Event(Control::PrimaryTile);
	event.hasTile = true;
	event.tile = tile;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputFrame2D InputFrame(
	std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> events)
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.bindingContext.input.worldInputEnabled = true;
	frame.events = events;
	return frame;
}

iggy::runtime::RuntimeGameplayProductPresentationCameraConfig CameraConfig()
{
	iggy::runtime::RuntimeGameplayProductPresentationCameraConfig config;
	config.fallbackCamera = { { 1.0F, 1.0F } };
	config.cameraView = { { 2.0F, 2.0F }, 1.0F };
	config.includeNpcCommands = false;
	config.rig.follow = { 10.0F, 0.0F };
	return config;
}

iggy::LevelTileRenderChunkCache BuildCache(const iggy::LevelTileMap &map)
{
	const iggy::LevelTileRenderChunkCacheBuildResult result =
		iggy::LevelTileRenderChunkCacheBuilder {}.build(
			map,
			{ 2, 1, { { WalkableMaterial, BlockedMaterial }, 4 } });
	Expect(result.built, "frame-request render cache should build");
	return result.cache;
}

iggy::runtime::RuntimeGameplayProductFrameRequestResult Run(
	const iggy::runtime::RuntimeGameplayProductFrameRequestInput &input)
{
	return iggy::runtime::RuntimeGameplayProductFrameRequest {}.run(input);
}

bool SameInputFrame(
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &actual,
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &expected)
{
	return actual.bindingContext.input.worldInputEnabled ==
			expected.bindingContext.input.worldInputEnabled
		&& actual.bindingContext.hasSelectedTargetId ==
			expected.bindingContext.hasSelectedTargetId
		&& actual.bindingContext.selectedTargetId ==
			expected.bindingContext.selectedTargetId
		&& actual.events.size() == expected.events.size();
}

void TestReadyFocusedRequestRunsCameraAndFrameOnce()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = ReadyState();
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	input.presentationCamera = CameraConfig();

	const auto result = Run(input);

	Expect(result.status == RequestStatus::Stepped,
		"ready request should report stepped status");
	Expect(result.presentationCamera.status == CameraStatus::Initialized,
		"ready request should build presentation camera before stepping");
	Expect(result.frame.status == FrameStatus::Stepped,
		"ready request should preserve play-mode frame result");
	Expect(result.frame.surface.status == SurfaceStatus::Stepped,
		"ready request should preserve play-surface result");
	Expect(result.frame.surface.presentation.status == PresentationStatus::Rendered,
		"ready request should render through play-mode frame");
	Expect(result.state.loop.nextFrameIndex == input.state.loop.nextFrameIndex + 1,
		"ready request should advance one frame");
	Expect(iggy::playerTile(result.state.loop.currentState.session.player) ==
			iggy::TileCoord { 2, 0 },
		"ready request should apply supplied tile input through existing runtime");
	Expect(result.inputEventCount == 1,
		"ready request should project input event count");
	Expect(result.ignoredInputEventCount == 0,
		"focused ready request should not ignore input events");
	Expect(result.frame.surface.step.frame.acceptedCommandCount == 1,
		"ready request should preserve nested accepted command count");
}

void TestNotLoadedRequestUsesCameraFallbackAndSkipsWork()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state.hasInputFocus = true;
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }), Event(Control::Wait) });
	input.presentationCamera = CameraConfig();
	input.presentationCamera.hasPreviousCamera = true;
	input.presentationCamera.previousCamera = { { 7.0F, 8.0F } };

	const auto result = Run(input);

	Expect(result.status == RequestStatus::NotLoaded,
		"not-loaded request should report not loaded");
	Expect(result.presentationCamera.status == CameraStatus::NotLoaded,
		"not-loaded request should preserve camera policy status");
	Expect(result.presentationCamera.usedPrevious,
		"not-loaded request should prefer previous camera when supplied");
	Expect(result.frame.status == FrameStatus::NotLoaded,
		"not-loaded request should preserve play-mode frame status");
	Expect(result.frame.surface.inputAdapter.eventCount == 0 &&
			result.frame.surface.playerBinding.actionCount == 0,
		"not-loaded request should skip adapter and binding through surface");
	Expect(!result.state.loop.loaded,
		"not-loaded request should keep returned state not loaded");
	Expect(result.inputEventCount == 2,
		"not-loaded request should count input events");
	Expect(result.ignoredInputEventCount == 2,
		"not-loaded request should surface ignored input count");
}

void TestNoFrameRequestRunsCameraAndPresentsCurrentState()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = ReadyState(1);
	input.state.loop.nextFrameIndex = input.state.loop.scenario.frames.size();
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	input.presentationCamera = CameraConfig();

	const auto result = Run(input);

	Expect(result.status == RequestStatus::NoFrameAvailable,
		"exhausted request should report no frame available");
	Expect(result.presentationCamera.status == CameraStatus::Initialized,
		"exhausted request should still run camera policy");
	Expect(result.frame.status == FrameStatus::NoFrameAvailable,
		"exhausted request should preserve play-mode no-frame status");
	Expect(result.frame.surface.inputAdapter.eventCount == 0 &&
			result.frame.surface.playerBinding.actionCount == 0,
		"exhausted request should skip adapter and binding through surface");
	Expect(result.frame.surface.presentation.status == PresentationStatus::Rendered,
		"exhausted loaded request should still present current state");
	Expect(result.state.loop.nextFrameIndex == input.state.loop.nextFrameIndex,
		"exhausted request should preserve frame index");
	Expect(result.ignoredInputEventCount == 1,
		"exhausted request should surface ignored input count");
}

void TestFreePlayNoFrameRequestRunsInputWithoutAdvancingFrameCursor()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = ReadyState(1);
	input.state.loop.nextFrameIndex = input.state.loop.scenario.frames.size();
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	input.allowFreePlayFrameWhenNoFrameAvailable = true;
	input.presentationCamera = CameraConfig();

	const auto result = Run(input);

	Expect(result.status == RequestStatus::Stepped,
		"free-play exhausted request should report stepped");
	Expect(result.presentationCamera.status == CameraStatus::Initialized,
		"free-play exhausted request should still run camera policy");
	Expect(result.frame.status == FrameStatus::Stepped,
		"free-play exhausted request should preserve play-mode stepped status");
	Expect(result.frame.surface.status == SurfaceStatus::Stepped,
		"free-play exhausted request should preserve play-surface stepped status");
	Expect(result.frame.surface.inputAdapter.eventCount == 1 &&
			result.frame.surface.playerBinding.emittedIntentCount == 1,
		"free-play exhausted request should map and bind supplied input");
	Expect(result.frame.surface.step.frame.acceptedCommandCount == 1,
		"free-play exhausted request should apply player input");
	Expect(result.state.loop.nextFrameIndex == input.state.loop.nextFrameIndex,
		"free-play exhausted request should not advance frame cursor");
	Expect(iggy::playerTile(result.state.loop.currentState.session.player) ==
			iggy::TileCoord { 2, 0 },
		"free-play exhausted request should update player through runtime");
	Expect(result.ignoredInputEventCount == 0,
		"free-play exhausted focused request should not ignore input");
}

void TestUnfocusedRequestIgnoresInputButConsumesFrame()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = ReadyState();
	input.state.hasInputFocus = false;
	input.inputFrame = InputFrame({ TileEvent({ 4, 0 }), Event(Control::Wait) });
	input.inputFrame.bindingContext.input.worldInputEnabled = false;
	input.presentationCamera = CameraConfig();
	const iggy::TileCoord startingTile =
		iggy::playerTile(input.state.loop.currentState.session.player);

	const auto result = Run(input);

	Expect(result.status == RequestStatus::Stepped,
		"unfocused request should still step an available frame");
	Expect(!result.state.hasInputFocus,
		"unfocused request should preserve focus bit");
	Expect(result.ignoredInputEventCount == 2,
		"unfocused request should ignore transient input events");
	Expect(result.frame.surface.playerBinding.emittedIntentCount == 0,
		"unfocused request should bind no player intents");
	Expect(!result.frame.surface.step.frame.input.playerFrame.playerInputContext.worldInputEnabled,
		"unfocused request should carry binding context override");
	Expect(result.state.loop.nextFrameIndex == 1,
		"unfocused request should consume exactly one frame");
	Expect(iggy::playerTile(result.state.loop.currentState.session.player) ==
			startingTile,
		"unfocused request should not apply ignored move event");
}

void TestPreviousCameraCanBeFedIntoNextRequest()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput first;
	first.state = ReadyState(2);
	first.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	first.presentationCamera = CameraConfig();

	const auto firstResult = Run(first);

	iggy::runtime::RuntimeGameplayProductFrameRequestInput second;
	second.state = firstResult.state;
	second.inputFrame = InputFrame({ TileEvent({ 3, 0 }) });
	second.presentationCamera = CameraConfig();
	second.presentationCamera.hasPreviousCamera = true;
	second.presentationCamera.previousCamera =
		firstResult.presentationCamera.presentationCamera;
	second.presentationCamera.rig.follow = { 0.5F, 0.0F };

	const auto secondResult = Run(second);
	const iggy::CameraRigResult expected = iggy::CameraRig {}.update(
		second.presentationCamera.previousCamera,
		second.state.loop.currentState.session.player.position,
		second.presentationCamera.rig);

	Expect(firstResult.presentationCamera.status == CameraStatus::Initialized,
		"first request should initialize camera from player");
	Expect(secondResult.presentationCamera.status == CameraStatus::FollowedPlayer,
		"second request should use previous camera and follow current player");
	Expect(secondResult.presentationCamera.usedPrevious,
		"second request should report previous-camera use");
	Expect(NearVec(
			   secondResult.presentationCamera.presentationCamera.position,
			   expected.state.position),
		"second request camera should match CameraRig from previous camera");
	Expect(secondResult.state.loop.nextFrameIndex == 2,
		"second request should consume the second frame");
}

void TestInputsAndNestedResultsArePreserved()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = ReadyState();
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	input.inputFrame.bindingContext.hasSelectedTargetId = true;
	input.inputFrame.bindingContext.selectedTargetId =
		iggy::ResourceId { "target:selected" };
	input.presentationCamera = CameraConfig();
	iggy::LevelTileRenderChunkCache cache =
		BuildCache(input.state.loop.currentState.session.level.map);
	input.presentationCamera.useTileChunkCache = true;
	input.presentationCamera.tileChunkCache = &cache;

	const iggy::runtime::RuntimeGameplayProductPlayModeState stateBefore =
		input.state;
	const iggy::runtime::RuntimeGameplayProductInputFrame2D inputFrameBefore =
		input.inputFrame;
	const auto cameraConfigBefore = input.presentationCamera;
	const std::size_t cacheChunkCount = cache.chunks.size();

	const auto result = Run(input);

	Expect(input.state.loop.nextFrameIndex == stateBefore.loop.nextFrameIndex,
		"frame request should not mutate input play state");
	Expect(NearVec(
			   input.state.loop.currentState.session.player.position,
			   stateBefore.loop.currentState.session.player.position),
		"frame request should not mutate input current state");
	Expect(SameInputFrame(input.inputFrame, inputFrameBefore),
		"frame request should not mutate input frame");
	Expect(input.presentationCamera.hasPreviousCamera ==
			cameraConfigBefore.hasPreviousCamera,
		"frame request should not mutate camera config previous flag");
	Expect(input.presentationCamera.tileChunkCache ==
			cameraConfigBefore.tileChunkCache,
		"frame request should not mutate camera config cache pointer");
	Expect(cache.chunks.size() == cacheChunkCount,
		"frame request should not mutate cache target");
	Expect(result.presentationCamera.levelRenderConfig.tileChunkCache == &cache,
		"frame request should preserve nested camera render config");
	Expect(result.frame.surface.presentation.status == PresentationStatus::Rendered,
		"frame request should preserve full play-mode frame result");
	Expect(result.frame.surface.step.frame.acceptedCommandCount == 1,
		"frame request should preserve nested frame counts");
}

} // namespace

int main()
{
	TestReadyFocusedRequestRunsCameraAndFrameOnce();
	TestNotLoadedRequestUsesCameraFallbackAndSkipsWork();
	TestNoFrameRequestRunsCameraAndPresentsCurrentState();
	TestFreePlayNoFrameRequestRunsInputWithoutAdvancingFrameCursor();
	TestUnfocusedRequestIgnoresInputButConsumesFrame();
	TestPreviousCameraCanBeFedIntoNextRequest();
	TestInputsAndNestedResultsArePreserved();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
