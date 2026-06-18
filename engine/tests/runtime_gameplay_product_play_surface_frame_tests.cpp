#include "runtime/RuntimeGameplayProductPlaySurfaceFrame.hpp"

#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameBounds;

using PlayStatus =
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameStatus;
using PresentationStatus =
	iggy::runtime::RuntimeGameplayProductPresentationFrameStatus;
using Control = iggy::runtime::RuntimeGameplayProductInputControl2D;
using IssueCode = iggy::runtime::RuntimeGameplayProductInputAdapterIssueCode;
using BindingIssueCode = iggy::PlayerInputBindingIssueCode;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };
const iggy::ResourceId NpcMaterial { "material:npc" };

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

iggy::runtime::RuntimeGameplayProductLoopBuildResult BuildFixture(
	const char *name)
{
	const iggy::runtime::RuntimeGameplayProductScenarioLoadResult load =
		iggy::runtime::RuntimeGameplayProductScenarioLoader {}.load(
			FixturePath(name));
	return iggy::runtime::RuntimeGameplayProductLoop {}.build(load);
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

iggy::runtime::RuntimeGameplayProductInputEvent2D TargetlessInteract()
{
	return Event(Control::Interact);
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	return { iggy::test::MapFromRows(rows), {} };
}

iggy::runtime::RuntimeGameplayProductLoopState UnloadedState()
{
	iggy::runtime::RuntimeGameplayProductLoopState state;
	state.loaded = false;
	state.currentState.session.level = Level({ "..", ".." });
	return state;
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
	int chunkHeight = 2,
	int layer = 4)
{
	const iggy::LevelTileRenderChunkCacheBuildResult result =
		iggy::LevelTileRenderChunkCacheBuilder {}.build(
			map,
			{ chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } });
	Expect(result.built, "play-surface test cache should build");
	return result.cache;
}

iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameResult BuildSurface(
	const iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameInput &input)
{
	return iggy::runtime::RuntimeGameplayProductPlaySurfaceFrame {}.build(input);
}

bool SameContext(
	const iggy::PlayerInputBindingContext2D &actual,
	const iggy::PlayerInputBindingContext2D &expected)
{
	return actual.input.playerControlEnabled ==
			expected.input.playerControlEnabled
		&& actual.input.worldInputEnabled ==
			expected.input.worldInputEnabled
		&& actual.input.interactionEnabled ==
			expected.input.interactionEnabled
		&& actual.input.cancelEnabled == expected.input.cancelEnabled
		&& actual.hasCurrentPlayerTile == expected.hasCurrentPlayerTile
		&& actual.currentPlayerTile == expected.currentPlayerTile
		&& actual.hasSelectedTargetId == expected.hasSelectedTargetId
		&& actual.selectedTargetId == expected.selectedTargetId
		&& actual.hasHoveredTargetId == expected.hasHoveredTargetId
		&& actual.hoveredTargetId == expected.hoveredTargetId;
}

bool SameEvent(
	const iggy::runtime::RuntimeGameplayProductInputEvent2D &actual,
	const iggy::runtime::RuntimeGameplayProductInputEvent2D &expected)
{
	return actual.control == expected.control && actual.kind == expected.kind
		&& actual.hasTile == expected.hasTile && actual.tile == expected.tile
		&& actual.hasWorldPoint == expected.hasWorldPoint
		&& NearVec(actual.worldPoint, expected.worldPoint)
		&& actual.hasTargetId == expected.hasTargetId
		&& actual.targetId == expected.targetId;
}

bool SameEvents(
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> &actual,
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D>
		&expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameEvent(actual[index], expected[index]))
			return false;
	}
	return true;
}

bool SameInputFrame(
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &actual,
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &expected)
{
	return SameContext(actual.bindingContext, expected.bindingContext)
		&& SameEvents(actual.events, expected.events);
}

bool SameProductStateShape(
	const iggy::runtime::RuntimeGameplayProductLoopState &actual,
	const iggy::runtime::RuntimeGameplayProductLoopState &expected)
{
	return actual.loaded == expected.loaded
		&& actual.nextFrameIndex == expected.nextFrameIndex
		&& actual.scenario.frames.size() == expected.scenario.frames.size()
		&& actual.currentState.session.level.map.width ==
			expected.currentState.session.level.map.width
		&& actual.currentState.session.level.map.height ==
			expected.currentState.session.level.map.height
		&& actual.currentState.session.level.map.tiles.size() ==
			expected.currentState.session.level.map.tiles.size();
}

void TestNotLoadedSkipsInputAndStep()
{
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameInput input;
	input.state = UnloadedState();
	input.inputFrame.events = {
		Event(Control::MoveEast),
		TileEvent({ 2, 0 }),
	};
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameResult result =
		BuildSurface(input);

	Expect(result.status == PlayStatus::NotLoaded,
		"not-loaded play surface should report NotLoaded");
	Expect(result.ignoredInputEventCount == 2,
		"not-loaded play surface should ignore all transient input events");
	Expect(result.inputAdapter.eventCount == 0 &&
			result.inputAdapter.actions.empty(),
		"not-loaded play surface should skip input adapter");
	Expect(result.playerBinding.actionCount == 0 &&
			result.playerBinding.intents.empty(),
		"not-loaded play surface should skip player binding");
	Expect(result.step.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::NotLoaded,
		"not-loaded play surface should not step product loop");
	Expect(result.presentation.status == PresentationStatus::NotLoaded,
		"not-loaded play surface should delegate presentation NotLoaded");
	Expect(NearVec(
			   result.presentation.presentationCamera.position,
			   input.presentationCamera.position),
		"not-loaded presentation should copy camera");
	Expect(result.presentation.levelFrame.commands.commands.empty(),
		"not-loaded presentation should leave render frame empty");
}

void TestNoFrameAvailableSkipsInputAndRendersCurrentState()
{
	iggy::runtime::RuntimeGameplayProductLoopBuildResult build =
		BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameInput input;
	input.state = build.state;
	input.state.nextFrameIndex = input.state.scenario.frames.size();
	input.inputFrame.events = {
		Event(Control::MoveEast),
		Event(Control::Wait),
	};
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameResult result =
		BuildSurface(input);

	Expect(result.status == PlayStatus::NoFrameAvailable,
		"exhausted play surface should report NoFrameAvailable");
	Expect(result.ignoredInputEventCount == 2,
		"exhausted play surface should ignore all transient input events");
	Expect(result.inputAdapter.eventCount == 0 &&
			result.playerBinding.actionCount == 0,
		"exhausted play surface should skip adapter and binding");
	Expect(result.step.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::NoFrameAvailable,
		"exhausted play surface should synthesize no-frame step status");
	Expect(result.step.frameIndex == input.state.nextFrameIndex,
		"exhausted play surface should preserve exhausted frame index");
	Expect(result.step.state.nextFrameIndex == input.state.nextFrameIndex,
		"exhausted play surface should not consume a frame");
	Expect(result.presentation.status == PresentationStatus::Rendered,
		"exhausted loaded play surface should still render current state");
	Expect(!result.presentation.levelFrame.commands.commands.empty(),
		"exhausted loaded presentation should render level commands");
}

void TestFocusedPathMapsBindsStepsAndPresentsPostStep()
{
	const iggy::runtime::RuntimeGameplayProductLoopBuildResult build =
		BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameInput input;
	input.state = build.state;
	input.inputFrame.events = { TileEvent({ 4, 1 }) };
	input.inputFrame.bindingContext.input.worldInputEnabled = true;
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameResult result =
		BuildSurface(input);

	Expect(result.status == PlayStatus::Stepped,
		"focused play surface should step one frame");
	Expect(result.inputAdapter.eventCount == 1 &&
			result.inputAdapter.emittedActionCount == 1,
		"focused play surface should map input events to actions");
	Expect(result.playerBinding.actionCount == 1 &&
			result.playerBinding.emittedIntentCount == 1,
		"focused play surface should bind actions to intents");
	Expect(result.step.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"focused play surface should call product loop step");
	Expect(result.step.state.nextFrameIndex == input.state.nextFrameIndex + 1,
		"focused play surface should advance one frame through product loop");
	Expect(result.step.frame.acceptedCommandCount == 1,
		"focused play surface should feed valid player intent into loop");
	Expect(iggy::playerTile(result.step.state.currentState.session.player) ==
			iggy::TileCoord { 4, 1 },
		"focused play surface should move player through existing gameplay");
	Expect(result.step.frame.input.playerFrame.playerInputContext.worldInputEnabled,
		"focused play surface should pass binding context as loop override");
	Expect(result.presentation.status == PresentationStatus::Rendered,
		"focused play surface should present post-step state");
}

void TestFocusedPathSurfacesAdapterAndBindingIssues()
{
	const iggy::runtime::RuntimeGameplayProductLoopBuildResult build =
		BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameInput input;
	input.state = build.state;
	input.inputFrame.events = {
		Event(Control::PrimaryTile),
		TileEvent({ 4, 1 }),
		TargetlessInteract(),
	};
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config();

	const iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameResult result =
		BuildSurface(input);

	Expect(result.status == PlayStatus::Stepped,
		"issue path should still step when valid intents remain");
	Expect(result.inputAdapter.hasIssues() &&
			result.inputAdapter.issueCount == 1,
		"issue path should surface adapter issue");
	if (!result.inputAdapter.issues.empty()) {
		Expect(result.inputAdapter.issues[0].code == IssueCode::MissingTile,
			"missing PrimaryTile payload should surface MissingTile");
	}
	Expect(result.playerBinding.hasIssues() &&
			result.playerBinding.issueCount == 1,
		"issue path should surface binding issue");
	if (!result.playerBinding.issues.empty()) {
		Expect(result.playerBinding.issues[0].code ==
				BindingIssueCode::MissingTarget,
			"targetless interact should surface binding MissingTarget");
	}
	Expect(result.playerBinding.emittedIntentCount == 1,
		"valid emitted intent should remain after issue filtering");
	Expect(result.step.frame.acceptedCommandCount == 1,
		"valid emitted intent should still reach product loop");
	Expect(iggy::playerTile(result.step.state.currentState.session.player) ==
			iggy::TileCoord { 4, 1 },
		"valid emitted intent should still move player");
}

void TestUnfocusedPathIgnoresEventsButStepsWithContextOverride()
{
	const iggy::runtime::RuntimeGameplayProductLoopBuildResult build =
		BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameInput input;
	input.state = build.state;
	input.hasInputFocus = false;
	input.inputFrame.events = {
		TileEvent({ 4, 1 }),
		Event(Control::Interact),
	};
	input.inputFrame.bindingContext.input.worldInputEnabled = false;
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config();
	const iggy::TileCoord startingTile =
		iggy::playerTile(input.state.currentState.session.player);

	const iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameResult result =
		BuildSurface(input);

	Expect(result.status == PlayStatus::Stepped,
		"unfocused play surface should still step one frame");
	Expect(result.ignoredInputEventCount == 2,
		"unfocused play surface should ignore all transient input events");
	Expect(result.inputAdapter.eventCount == 0 &&
			result.inputAdapter.emittedActionCount == 0,
		"unfocused play surface should run adapter on empty events");
	Expect(result.playerBinding.actionCount == 0 &&
			result.playerBinding.emittedIntentCount == 0,
		"unfocused play surface should bind empty actions");
	Expect(result.step.frame.input.playerFrame.playerIntents.empty(),
		"unfocused play surface should step with empty player intents");
	Expect(!result.step.frame.input.playerFrame.playerInputContext.worldInputEnabled,
		"unfocused play surface should still carry input context override");
	Expect(result.step.state.nextFrameIndex == input.state.nextFrameIndex + 1,
		"unfocused play surface should consume one available frame");
	Expect(iggy::playerTile(result.step.state.currentState.session.player) ==
			startingTile,
		"unfocused play surface should not apply ignored player movement");
	Expect(result.presentation.status == PresentationStatus::Rendered,
		"unfocused play surface should present post-step state");
}

void TestInputObjectsAndCacheAreNotMutated()
{
	const iggy::runtime::RuntimeGameplayProductLoopBuildResult build =
		BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameInput input;
	input.state = build.state;
	input.inputFrame.events = { TileEvent({ 4, 1 }) };
	input.inputFrame.bindingContext.hasSelectedTargetId = true;
	input.inputFrame.bindingContext.selectedTargetId =
		iggy::ResourceId { "target:selected" };
	input.presentationCamera = { { 1.0F, 1.0F } };
	iggy::LevelTileRenderChunkCache cache = BuildCache(
		input.state.currentState.session.level.map);
	const std::size_t cacheChunkCount = cache.chunks.size();
	const std::size_t cacheCommandCount =
		cache.chunks.empty() ? 0 : cache.chunks[0].commands.commands.size();
	input.levelRenderConfig = Config();
	input.levelRenderConfig.useTileChunkCache = true;
	input.levelRenderConfig.tileChunkCache = &cache;

	const iggy::runtime::RuntimeGameplayProductLoopState stateBefore =
		input.state;
	const iggy::runtime::RuntimeGameplayProductInputFrame2D inputFrameBefore =
		input.inputFrame;
	const iggy::CameraState cameraBefore = input.presentationCamera;
	const bool useCacheBefore = input.levelRenderConfig.useTileChunkCache;
	const iggy::LevelTileRenderChunkCache *cachePointerBefore =
		input.levelRenderConfig.tileChunkCache;

	const iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameResult result =
		BuildSurface(input);

	Expect(result.status == PlayStatus::Stepped,
		"immutability setup should step");
	Expect(SameProductStateShape(input.state, stateBefore),
		"play surface should not mutate input product loop state");
	Expect(NearVec(
			   input.state.currentState.session.player.position,
			   stateBefore.currentState.session.player.position),
		"play surface should not mutate input current player state");
	Expect(SameInputFrame(input.inputFrame, inputFrameBefore),
		"play surface should not mutate input frame events or context");
	Expect(NearVec(input.presentationCamera.position, cameraBefore.position),
		"play surface should not mutate caller-owned camera");
	Expect(input.levelRenderConfig.useTileChunkCache == useCacheBefore &&
			input.levelRenderConfig.tileChunkCache == cachePointerBefore,
		"play surface should not mutate render config or cache pointer");
	Expect(cache.chunks.size() == cacheChunkCount,
		"play surface should not mutate cache chunk count");
	if (!cache.chunks.empty()) {
		Expect(cache.chunks[0].commands.commands.size() == cacheCommandCount,
			"play surface should not mutate cached commands");
	}
}

} // namespace

int main()
{
	TestNotLoadedSkipsInputAndStep();
	TestNoFrameAvailableSkipsInputAndRendersCurrentState();
	TestFocusedPathMapsBindsStepsAndPresentsPostStep();
	TestFocusedPathSurfacesAdapterAndBindingIssues();
	TestUnfocusedPathIgnoresEventsButStepsWithContextOverride();
	TestInputObjectsAndCacheAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
