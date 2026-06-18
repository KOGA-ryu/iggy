#include "runtime/RuntimeGameplayProductPlayMode.hpp"

#include <cstdlib>
#include <string_view>
#include <vector>

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

using BuildStatus = iggy::runtime::RuntimeGameplayProductPlayModeBuildStatus;
using FrameStatus = iggy::runtime::RuntimeGameplayProductPlayModeFrameStatus;
using LoopStatus = iggy::runtime::RuntimeGameplayProductLoopStatus;
using SurfaceStatus =
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameStatus;
using Control = iggy::runtime::RuntimeGameplayProductInputControl2D;

const iggy::ResourceId PlayerId { "player:play-mode" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };
const iggy::ResourceId NpcMaterial { "material:npc" };

iggy::LevelTileMap LevelMap(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = iggy::ResourceId { "level:play-mode" };
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
	Expect(result.built, "play-mode AI map should build");
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

iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame Frame(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame;
	frame.playerFrame = PlayerFrame(state);
	frame.movementMap = map;
	frame.aiMap = AiMap();
	frame.refreshAiMap = AiMap();
	return frame;
}

iggy::runtime::RuntimeGameplayScenarioFrame ScenarioFrame(
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame &frame)
{
	return { frame };
}

iggy::runtime::RuntimeGameplayProductLoopBuildResult ReadyLoopBuild(
	std::size_t frameCount = 2)
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);

	iggy::runtime::RuntimeGameplayProductLoopBuildResult build;
	build.status = LoopStatus::Ready;
	build.state.loaded = true;
	build.state.inputPath = "in-memory-play-mode";
	build.state.sourcePath = "in-memory-play-mode";
	build.state.initialState = state;
	build.state.currentState = state;
	build.state.scenario.initialState = state;
	for (std::size_t index = 0; index < frameCount; ++index)
		build.state.scenario.frames.push_back(ScenarioFrame(Frame(state, map)));
	return build;
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

iggy::LevelRenderFrame2DConfig Config(bool includeNpcCommands = false)
{
	return {
		{ { 2.0F, 2.0F }, 1.0F },
		{ { WalkableMaterial, BlockedMaterial }, 1 },
		{ NpcMaterial, { 1.0F, 1.0F }, { 0.5F, 0.5F }, 8 },
		includeNpcCommands,
	};
}

iggy::LevelTileRenderChunkCache BuildCache(
	const iggy::LevelTileMap &map,
	int chunkWidth = 2,
	int chunkHeight = 1,
	int layer = 4)
{
	const iggy::LevelTileRenderChunkCacheBuildResult result =
		iggy::LevelTileRenderChunkCacheBuilder {}.build(
			map,
			{ chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } });
	Expect(result.built, "play-mode render cache should build");
	return result.cache;
}

bool SameInputFrame(
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &actual,
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &expected)
{
	return actual.bindingContext.input.worldInputEnabled ==
			expected.bindingContext.input.worldInputEnabled
		&& actual.bindingContext.input.playerControlEnabled ==
			expected.bindingContext.input.playerControlEnabled
		&& actual.bindingContext.hasSelectedTargetId ==
			expected.bindingContext.hasSelectedTargetId
		&& actual.bindingContext.selectedTargetId ==
			expected.bindingContext.selectedTargetId
		&& actual.events.size() == expected.events.size();
}

iggy::runtime::RuntimeGameplayProductPlayModeBuildResult BuildPlayMode(
	const iggy::runtime::RuntimeGameplayProductLoopBuildResult &loop)
{
	return iggy::runtime::RuntimeGameplayProductPlayMode {}.build(loop);
}

iggy::runtime::RuntimeGameplayProductPlayModeFrameResult Frame(
	const iggy::runtime::RuntimeGameplayProductPlayModeFrameInput &input)
{
	return iggy::runtime::RuntimeGameplayProductPlayMode {}.frame(input);
}

void TestBuildFromReadyLoop()
{
	const iggy::runtime::RuntimeGameplayProductLoopBuildResult loop =
		ReadyLoopBuild();

	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult result =
		BuildPlayMode(loop);

	Expect(result.status == BuildStatus::Ready,
		"ready product loop should build ready play mode");
	Expect(result.state.loop.loaded,
		"ready play mode should copy loaded loop state");
	Expect(result.state.loop.sourcePath == loop.state.sourcePath,
		"ready play mode should copy loop source path");
	Expect(result.state.loop.scenario.frames.size() ==
			loop.state.scenario.frames.size(),
		"ready play mode should copy lowered scenario frames");
	Expect(NearVec(
			   result.state.loop.currentState.session.player.position,
			   loop.state.currentState.session.player.position),
		"ready play mode should copy current state");
	Expect(result.state.hasInputFocus,
		"ready play mode should default to input focus");
	Expect(result.loop.status == loop.status,
		"ready play mode should preserve nested loop build result");
}

void TestBuildFromFailedLoop()
{
	iggy::runtime::RuntimeGameplayProductLoopBuildResult loop;
	loop.status = LoopStatus::LoadFailed;
	loop.state.loaded = true;
	loop.state.nextFrameIndex = 7;

	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult result =
		BuildPlayMode(loop);

	Expect(result.status == BuildStatus::LoadFailed,
		"failed product loop should build failed play mode");
	Expect(result.loop.status == loop.status,
		"failed play mode should preserve nested loop build result");
	Expect(result.loop.state.loaded,
		"failed play mode should preserve nested loop payload");
	Expect(!result.state.loop.loaded,
		"failed play mode state should remain not loaded");
	Expect(result.state.loop.nextFrameIndex == 0,
		"failed play mode should not invent loop progress");
}

void TestFocusedFrameDelegatesAndCarriesReturnedLoop()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		BuildPlayMode(ReadyLoopBuild());
	iggy::runtime::RuntimeGameplayProductPlayModeFrameInput input;
	input.state = build.state;
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	input.presentationCamera = { { 1.0F, 0.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult result =
		Frame(input);

	Expect(result.status == FrameStatus::Stepped,
		"focused play mode should report stepped surface frame");
	Expect(result.surface.status == SurfaceStatus::Stepped,
		"focused play mode should delegate to play surface");
	Expect(result.surface.step.state.nextFrameIndex ==
			input.state.loop.nextFrameIndex + 1,
		"focused play mode should step exactly once");
	Expect(result.state.loop.nextFrameIndex ==
			result.surface.step.state.nextFrameIndex,
		"focused play mode should carry returned loop state");
	Expect(result.state.hasInputFocus == input.state.hasInputFocus,
		"focused play mode should preserve focus bit");
	Expect(iggy::playerTile(result.state.loop.currentState.session.player) ==
			iggy::TileCoord { 2, 0 },
		"focused play mode should expose post-step current state");
	Expect(result.surface.presentation.status ==
			iggy::runtime::RuntimeGameplayProductPresentationFrameStatus::Rendered,
		"focused play mode should present post-step state");
}

void TestConsecutiveFramesCarryStateForward()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		BuildPlayMode(ReadyLoopBuild(2));

	iggy::runtime::RuntimeGameplayProductPlayModeFrameInput first;
	first.state = build.state;
	first.inputFrame = InputFrame({ TileEvent({ 1, 0 }) });
	first.presentationCamera = { { 1.0F, 0.0F } };
	first.levelRenderConfig = Config();
	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult firstResult =
		Frame(first);

	iggy::runtime::RuntimeGameplayProductPlayModeFrameInput second;
	second.state = firstResult.state;
	second.inputFrame = InputFrame({ TileEvent({ 3, 0 }) });
	second.presentationCamera = first.presentationCamera;
	second.levelRenderConfig = first.levelRenderConfig;
	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult secondResult =
		Frame(second);

	Expect(firstResult.status == FrameStatus::Stepped &&
			secondResult.status == FrameStatus::Stepped,
		"consecutive play mode frames should both step");
	Expect(firstResult.state.loop.nextFrameIndex == 1,
		"first play mode frame should consume first scenario frame");
	Expect(secondResult.state.loop.nextFrameIndex == 2,
		"second play mode frame should consume second scenario frame");
	Expect(iggy::playerTile(secondResult.state.loop.currentState.session.player) ==
			iggy::TileCoord { 3, 0 },
		"second play mode frame should start from returned first state");
}

void TestFocusTogglePersistsAndIgnoresTransientEvents()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		BuildPlayMode(ReadyLoopBuild(2));
	const iggy::runtime::RuntimeGameplayProductPlayModeState unfocused =
		iggy::runtime::RuntimeGameplayProductPlayMode {}.withInputFocus(
			build.state,
			false);

	iggy::runtime::RuntimeGameplayProductPlayModeFrameInput input;
	input.state = unfocused;
	input.inputFrame = InputFrame({ TileEvent({ 4, 0 }), Event(Control::Wait) });
	input.presentationCamera = { { 1.0F, 0.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult result =
		Frame(input);

	Expect(result.status == FrameStatus::Stepped,
		"unfocused play mode should still step an available frame");
	Expect(!result.state.hasInputFocus,
		"unfocused play mode should carry focus bit forward");
	Expect(result.surface.ignoredInputEventCount == 2,
		"unfocused play mode should let play surface ignore transient input");
	Expect(result.surface.playerBinding.emittedIntentCount == 0,
		"unfocused play mode should step with empty player intents");
	Expect(result.state.loop.nextFrameIndex == 1,
		"unfocused play mode should still consume one frame");
	Expect(iggy::playerTile(result.state.loop.currentState.session.player) ==
			iggy::TileCoord { 0, 0 },
		"unfocused play mode should not apply ignored move event");
}

void TestNoFramePreservesStateAndFocus()
{
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		BuildPlayMode(ReadyLoopBuild(1));
	build.state.loop.nextFrameIndex = build.state.loop.scenario.frames.size();
	build.state.hasInputFocus = false;

	iggy::runtime::RuntimeGameplayProductPlayModeFrameInput input;
	input.state = build.state;
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	input.presentationCamera = { { 1.0F, 0.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult result =
		Frame(input);

	Expect(result.status == FrameStatus::NoFrameAvailable,
		"exhausted play mode should report no frame available");
	Expect(result.state.loop.nextFrameIndex == input.state.loop.nextFrameIndex,
		"exhausted play mode should preserve loop frame index");
	Expect(!result.state.hasInputFocus,
		"exhausted play mode should preserve focus bit");
	Expect(result.surface.ignoredInputEventCount == 1,
		"exhausted play mode should ignore transient input through surface");
	Expect(result.surface.step.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::NoFrameAvailable,
		"exhausted play mode should surface no-frame step status");
}

void TestNotLoadedFramePreservesStateAndFocus()
{
	iggy::runtime::RuntimeGameplayProductPlayModeFrameInput input;
	input.state.hasInputFocus = false;
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }), Event(Control::Wait) });
	input.presentationCamera = { { 1.0F, 0.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult result =
		Frame(input);

	Expect(result.status == FrameStatus::NotLoaded,
		"not-loaded play mode should report NotLoaded");
	Expect(!result.state.loop.loaded,
		"not-loaded play mode should preserve unloaded loop state");
	Expect(!result.state.hasInputFocus,
		"not-loaded play mode should preserve focus bit");
	Expect(result.surface.ignoredInputEventCount == 2,
		"not-loaded play mode should ignore transient input through surface");
	Expect(result.surface.step.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::NotLoaded,
		"not-loaded play mode should not step the product loop");
	Expect(result.surface.presentation.status ==
			iggy::runtime::RuntimeGameplayProductPresentationFrameStatus::NotLoaded,
		"not-loaded play mode should surface not-loaded presentation");
}

void TestInputObjectsAndCacheAreNotMutated()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		BuildPlayMode(ReadyLoopBuild());
	iggy::runtime::RuntimeGameplayProductPlayModeFrameInput input;
	input.state = build.state;
	input.inputFrame = InputFrame({ TileEvent({ 2, 0 }) });
	input.inputFrame.bindingContext.hasSelectedTargetId = true;
	input.inputFrame.bindingContext.selectedTargetId =
		iggy::ResourceId { "target:selected" };
	input.presentationCamera = { { 1.0F, 0.0F } };
	iggy::LevelTileRenderChunkCache cache = BuildCache(
		input.state.loop.currentState.session.level.map);
	const std::size_t cacheChunkCount = cache.chunks.size();
	input.levelRenderConfig = Config();
	input.levelRenderConfig.useTileChunkCache = true;
	input.levelRenderConfig.tileChunkCache = &cache;

	const iggy::runtime::RuntimeGameplayProductPlayModeState stateBefore =
		input.state;
	const iggy::runtime::RuntimeGameplayProductInputFrame2D inputFrameBefore =
		input.inputFrame;
	const iggy::CameraState cameraBefore = input.presentationCamera;
	const bool useCacheBefore = input.levelRenderConfig.useTileChunkCache;
	const iggy::LevelTileRenderChunkCache *cachePointerBefore =
		input.levelRenderConfig.tileChunkCache;

	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult result =
		Frame(input);

	Expect(result.status == FrameStatus::Stepped,
		"immutability setup should step");
	Expect(input.state.loop.nextFrameIndex == stateBefore.loop.nextFrameIndex,
		"play mode should not mutate input loop state");
	Expect(NearVec(
			   input.state.loop.currentState.session.player.position,
			   stateBefore.loop.currentState.session.player.position),
		"play mode should not mutate input current state");
	Expect(input.state.hasInputFocus == stateBefore.hasInputFocus,
		"play mode should not mutate input focus bit");
	Expect(SameInputFrame(input.inputFrame, inputFrameBefore),
		"play mode should not mutate input events or context");
	Expect(NearVec(input.presentationCamera.position, cameraBefore.position),
		"play mode should not mutate caller camera");
	Expect(input.levelRenderConfig.useTileChunkCache == useCacheBefore &&
			input.levelRenderConfig.tileChunkCache == cachePointerBefore,
		"play mode should not mutate render config or cache pointer");
	Expect(cache.chunks.size() == cacheChunkCount,
		"play mode should not mutate render cache");
}

} // namespace

int main()
{
	TestBuildFromReadyLoop();
	TestBuildFromFailedLoop();
	TestFocusedFrameDelegatesAndCarriesReturnedLoop();
	TestConsecutiveFramesCarryStateForward();
	TestFocusTogglePersistsAndIgnoresTransientEvents();
	TestNoFramePreservesStateAndFocus();
	TestNotLoadedFramePreservesStateAndFocus();
	TestInputObjectsAndCacheAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
