#include "runtime/RuntimeGameplayProductInputContext.hpp"

#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayProductFrameRequest.hpp"
#include "scene/player/PlayerAgentState.hpp"
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

using ContextStatus =
	iggy::runtime::RuntimeGameplayProductInputContextStatus;
using RequestStatus =
	iggy::runtime::RuntimeGameplayProductFrameRequestStatus;
using LoopStatus = iggy::runtime::RuntimeGameplayProductLoopStatus;
using Control = iggy::runtime::RuntimeGameplayProductInputControl2D;

const iggy::ResourceId PlayerId { "player:input-context" };

iggy::LevelTileMap LevelMap(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = iggy::ResourceId { "level:input-context" };
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
	Expect(result.built, "input-context AI map should build");
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
	std::size_t frameCount = 1)
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);

	iggy::runtime::RuntimeGameplayProductLoopBuildResult loop;
	loop.status = LoopStatus::Ready;
	loop.state.loaded = true;
	loop.state.sourcePath = "in-memory-input-context";
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

iggy::runtime::RuntimeGameplayProductInputFrame2D InputFrame(
	std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> events)
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = events;
	return frame;
}

iggy::runtime::RuntimeGameplayProductPresentationCameraConfig CameraConfig()
{
	iggy::runtime::RuntimeGameplayProductPresentationCameraConfig config;
	config.fallbackCamera = { { 1.0F, 1.0F } };
	config.cameraView = { { 2.0F, 2.0F }, 1.0F };
	config.rig.follow = { 10.0F, 0.0F };
	return config;
}

bool DefaultGates(const iggy::PlayerInputBindingContext2D &context)
{
	return context.input.playerControlEnabled &&
		context.input.worldInputEnabled && context.input.interactionEnabled &&
		context.input.cancelEnabled;
}

bool NoTargets(const iggy::PlayerInputBindingContext2D &context)
{
	return !context.hasSelectedTargetId && context.selectedTargetId.empty() &&
		!context.hasHoveredTargetId && context.hoveredTargetId.empty();
}

void TestNotLoadedReturnsDefaultContext()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state;

	const auto result =
		iggy::runtime::RuntimeGameplayProductInputContext {}.build(state);

	Expect(result.status == ContextStatus::NotLoaded,
		"not-loaded input context should report NotLoaded");
	Expect(DefaultGates(result.bindingContext),
		"not-loaded input context should preserve default gates");
	Expect(!result.bindingContext.hasCurrentPlayerTile,
		"not-loaded input context should not project current player tile");
	Expect(NoTargets(result.bindingContext),
		"not-loaded input context should not invent target ids");
}

void TestLoadedWithoutPlayerReturnsDefaultContext()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state = ReadyState();
	state.loop.currentState.session.hasPlayer = false;
	state.loop.currentState.session.player = {};

	const auto result =
		iggy::runtime::RuntimeGameplayProductInputContext {}.build(state);

	Expect(result.status == ContextStatus::LoadedWithoutPlayer,
		"loaded state without player should report LoadedWithoutPlayer");
	Expect(DefaultGates(result.bindingContext),
		"loaded state without player should preserve default gates");
	Expect(!result.bindingContext.hasCurrentPlayerTile,
		"loaded state without player should not project current player tile");
	Expect(NoTargets(result.bindingContext),
		"loaded state without player should not invent target ids");
}

void TestLoadedWithPlayerProjectsCurrentPlayerTile()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state = ReadyState();
	state.loop.currentState.session.player.position = { 2.75F, 3.25F };

	const auto result =
		iggy::runtime::RuntimeGameplayProductInputContext {}.build(state);

	Expect(result.status == ContextStatus::Projected,
		"loaded state with player should report Projected");
	Expect(DefaultGates(result.bindingContext),
		"projected input context should preserve default gates");
	Expect(result.bindingContext.hasCurrentPlayerTile,
		"projected input context should expose current player tile");
	Expect(result.bindingContext.currentPlayerTile ==
			iggy::playerTile(state.loop.currentState.session.player),
		"projected input context should use playerTile semantics");
	Expect(NoTargets(result.bindingContext),
		"projected input context should not invent target ids");
}

void TestNegativePlayerPositionUsesPlayerTileSemantics()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state = ReadyState();
	state.loop.currentState.session.player.position = { -0.25F, -1.1F };

	const auto result =
		iggy::runtime::RuntimeGameplayProductInputContext {}.build(state);

	Expect(result.status == ContextStatus::Projected,
		"negative-position loaded player should still project context");
	Expect(result.bindingContext.currentPlayerTile == iggy::TileCoord { -1, -2 },
		"projected input context should floor negative positions like playerTile");
}

void TestInputPlayStateIsNotMutated()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state = ReadyState();
	state.loop.currentState.session.player.position = { 2.75F, 3.25F };
	const iggy::runtime::RuntimeGameplayProductPlayModeState before = state;

	const auto result =
		iggy::runtime::RuntimeGameplayProductInputContext {}.build(state);

	Expect(result.status == ContextStatus::Projected,
		"immutability setup should project context");
	Expect(state.loop.loaded == before.loop.loaded &&
			state.loop.nextFrameIndex == before.loop.nextFrameIndex,
		"input context projection should not mutate loop metadata");
	Expect(NearVec(
			   state.loop.currentState.session.player.position,
			   before.loop.currentState.session.player.position),
		"input context projection should not mutate current player position");
}

void TestCardinalInputUsesProjectedContextThroughFrameRequest()
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = ReadyState();
	input.inputFrame = InputFrame({ Event(Control::MoveEast) });
	input.inputFrame.bindingContext =
		iggy::runtime::RuntimeGameplayProductInputContext {}
			.build(input.state)
			.bindingContext;
	input.presentationCamera = CameraConfig();

	const auto result =
		iggy::runtime::RuntimeGameplayProductFrameRequest {}.run(input);

	Expect(result.status == RequestStatus::Stepped,
		"projected-context cardinal request should step");
	Expect(result.frame.surface.inputAdapter.emittedActionCount == 1,
		"projected-context cardinal request should map input action");
	Expect(result.frame.surface.playerBinding.emittedIntentCount == 1,
		"projected-context cardinal request should bind movement intent");
	Expect(result.frame.surface.playerBinding.issueCount == 0,
		"projected-context cardinal request should not report missing tile");
	Expect(result.frame.surface.step.frame.acceptedCommandCount == 1,
		"projected-context cardinal request should accept movement command");
	Expect(iggy::playerTile(result.state.loop.currentState.session.player) ==
			iggy::TileCoord { 1, 0 },
		"projected-context cardinal request should move from current tile plus delta");
}

} // namespace

int main()
{
	TestNotLoadedReturnsDefaultContext();
	TestLoadedWithoutPlayerReturnsDefaultContext();
	TestLoadedWithPlayerProjectsCurrentPlayerTile();
	TestNegativePlayerPositionUsesPlayerTileSemantics();
	TestInputPlayStateIsNotMutated();
	TestCardinalInputUsesProjectedContextThroughFrameRequest();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
