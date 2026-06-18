#include "runtime/RuntimeGameplayProductInputAccumulator.hpp"

#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayProductFrameRequest.hpp"
#include "runtime/RuntimeGameplayProductInputContext.hpp"
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

using Control = iggy::runtime::RuntimeGameplayProductInputControl2D;
using Kind = iggy::runtime::RuntimeGameplayProductInputEventKind;
using RequestStatus =
	iggy::runtime::RuntimeGameplayProductFrameRequestStatus;
using LoopStatus = iggy::runtime::RuntimeGameplayProductLoopStatus;

const iggy::ResourceId PlayerId { "player:input-accumulator" };

iggy::runtime::RuntimeGameplayProductInputEvent2D Event(
	Control control,
	Kind kind = Kind::Pressed)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event;
	event.control = control;
	event.kind = kind;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputAccumulatorRecordResult Record(
	const iggy::runtime::RuntimeGameplayProductInputAccumulatorState &state,
	iggy::runtime::RuntimeGameplayProductInputEvent2D event)
{
	return iggy::runtime::RuntimeGameplayProductInputAccumulator {}.record(
		state,
		event);
}

iggy::runtime::RuntimeGameplayProductInputAccumulatorFrameResult BuildFrame(
	const iggy::runtime::RuntimeGameplayProductInputAccumulatorState &state,
	iggy::PlayerInputBindingContext2D context = {})
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorFrameInput input;
	input.state = state;
	input.bindingContext = context;
	return iggy::runtime::RuntimeGameplayProductInputAccumulator {}.buildFrame(
		input);
}

iggy::LevelTileMap LevelMap(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = iggy::ResourceId { "level:input-accumulator" };
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
	Expect(result.built, "input-accumulator AI map should build");
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
	loop.state.sourcePath = "in-memory-input-accumulator";
	loop.state.initialState = state;
	loop.state.currentState = state;
	loop.state.scenario.initialState = state;
	for (std::size_t index = 0; index < frameCount; ++index)
		loop.state.scenario.frames.push_back(ScenarioFrame(state, map));

	return iggy::runtime::RuntimeGameplayProductPlayMode {}.build(loop).state;
}

iggy::runtime::RuntimeGameplayProductPresentationCameraConfig CameraConfig()
{
	iggy::runtime::RuntimeGameplayProductPresentationCameraConfig config;
	config.fallbackCamera = { { 1.0F, 1.0F } };
	config.cameraView = { { 2.0F, 2.0F }, 1.0F };
	config.rig.follow = { 10.0F, 0.0F };
	return config;
}

iggy::runtime::RuntimeGameplayProductFrameRequestResult RunFrameRequest(
	iggy::runtime::RuntimeGameplayProductPlayModeState state,
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame)
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = state;
	input.inputFrame = frame;
	input.presentationCamera = CameraConfig();
	return iggy::runtime::RuntimeGameplayProductFrameRequest {}.run(input);
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

void TestMovementPressBuildsHeldPressedEvent()
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state;

	const auto record = Record(state, Event(Control::MoveEast));
	const auto frame = BuildFrame(record.state);

	Expect(record.changed, "movement press should change accumulator state");
	Expect(record.heldControlCount == 1,
		"movement press should mark one control held");
	Expect(frame.heldEventCount == 1 && frame.eventCount == 1,
		"held movement should emit one frame event");
	Expect(frame.frame.events[0].control == Control::MoveEast &&
			frame.frame.events[0].kind == Kind::Pressed,
		"held movement should emit ordinary pressed event");
}

void TestHeldMovementRepeatsUntilRelease()
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state =
		Record({}, Event(Control::MoveEast)).state;

	const auto first = BuildFrame(state);
	state = first.state;
	const auto second = BuildFrame(state);
	state = Record(state, Event(Control::MoveEast, Kind::Released)).state;
	const auto released = BuildFrame(state);

	Expect(first.eventCount == 1 && second.eventCount == 1,
		"held movement should emit on subsequent frames");
	Expect(released.eventCount == 0,
		"released held movement should not emit on later frames");
}

void TestDuplicateMovementPressDoesNotDuplicateHeldControl()
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state =
		Record({}, Event(Control::MoveEast)).state;

	const auto duplicate = Record(state, Event(Control::MoveEast));
	const auto frame = BuildFrame(duplicate.state);

	Expect(!duplicate.changed,
		"duplicate movement press should not change accumulator");
	Expect(duplicate.heldControlCount == 1,
		"duplicate movement press should preserve one held control");
	Expect(frame.eventCount == 1,
		"duplicate movement press should not emit duplicate movement events");
}

void TestReleaseOfNonHeldMovementIsNoOp()
{
	const auto record = Record({}, Event(Control::MoveWest, Kind::Released));
	const auto frame = BuildFrame(record.state);

	Expect(!record.changed,
		"release of non-held movement should not change accumulator");
	Expect(record.heldControlCount == 0,
		"release of non-held movement should keep no held controls");
	Expect(frame.eventCount == 0,
		"release of non-held movement should emit no frame events");
}

void TestOppositeCardinalsPreservePressOrder()
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state;
	state = Record(state, Event(Control::MoveEast)).state;
	state = Record(state, Event(Control::MoveWest)).state;

	const auto frame = BuildFrame(state);

	Expect(frame.eventCount == 2,
		"opposite held cardinals should both emit in v1");
	Expect(frame.frame.events[0].control == Control::MoveEast &&
			frame.frame.events[1].control == Control::MoveWest,
		"opposite held cardinals should preserve press order");
}

void TestOneShotPressEmitsOnceAndCanBePressedAgain()
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state =
		Record({}, Event(Control::Wait)).state;

	const auto first = BuildFrame(state);
	state = first.state;
	const auto second = BuildFrame(state);
	state = Record(state, Event(Control::Wait)).state;
	const auto third = BuildFrame(state);

	Expect(first.oneShotEventCount == 1 && first.eventCount == 1,
		"one-shot press should emit on next frame");
	Expect(second.eventCount == 0,
		"one-shot press should be consumed after one frame");
	Expect(third.oneShotEventCount == 1 && third.eventCount == 1,
		"re-pressed one-shot should emit again");
}

void TestOneShotReleaseIsNoOp()
{
	const auto record = Record({}, Event(Control::Cancel, Kind::Released));
	const auto frame = BuildFrame(record.state);

	Expect(!record.changed,
		"one-shot release should not change accumulator state");
	Expect(record.pendingOneShotCount == 0,
		"one-shot release should not add pending events");
	Expect(frame.eventCount == 0,
		"one-shot release should emit no frame events");
}

void TestPrimaryTilePressEmitsOnceWithPayloadPreserved()
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D tile =
		Event(Control::PrimaryTile);
	tile.hasTile = true;
	tile.tile = { 4, 5 };
	const auto record = Record({}, tile);

	const auto first = BuildFrame(record.state);
	const auto second = BuildFrame(first.state);

	Expect(record.changed && record.pendingOneShotCount == 1,
		"PrimaryTile press should add one pending one-shot event");
	Expect(first.oneShotEventCount == 1 && first.eventCount == 1,
		"PrimaryTile press should emit once on next frame");
	if (first.frame.events.size() == 1) {
		const auto &event = first.frame.events[0];
		Expect(event.control == Control::PrimaryTile &&
				event.kind == Kind::Pressed &&
				event.hasTile &&
				event.tile == iggy::TileCoord { 4, 5 },
			"PrimaryTile event should preserve tile payload");
	}
	Expect(second.eventCount == 0,
		"PrimaryTile one-shot should be drained after one frame");
}

void TestPrimaryPointPressEmitsOnceWithPayloadPreserved()
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D point =
		Event(Control::PrimaryPoint);
	point.hasWorldPoint = true;
	point.worldPoint = { 2.0F, 3.0F };
	const auto record = Record({}, point);

	const auto first = BuildFrame(record.state);
	const auto second = BuildFrame(first.state);

	Expect(record.changed && record.pendingOneShotCount == 1,
		"PrimaryPoint press should add one pending one-shot event");
	Expect(first.oneShotEventCount == 1 && first.eventCount == 1,
		"PrimaryPoint press should emit once on next frame");
	if (first.frame.events.size() == 1) {
		const auto &event = first.frame.events[0];
		Expect(event.control == Control::PrimaryPoint &&
				event.kind == Kind::Pressed &&
				event.hasWorldPoint &&
				NearVec(event.worldPoint, { 2.0F, 3.0F }),
			"PrimaryPoint event should preserve world-point payload");
	}
	Expect(second.eventCount == 0,
		"PrimaryPoint one-shot should be drained after one frame");
}

void TestPrimaryReleaseIsNoOp()
{
	auto pointRelease = Event(Control::PrimaryPoint, Kind::Released);
	pointRelease.hasWorldPoint = true;
	pointRelease.worldPoint = { 2.0F, 3.0F };
	auto tileRelease = Event(Control::PrimaryTile, Kind::Released);
	tileRelease.hasTile = true;
	tileRelease.tile = { 4, 5 };

	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state;
	const auto point = Record(state, pointRelease);
	state = point.state;
	const auto tile = Record(state, tileRelease);
	const auto frame = BuildFrame(tile.state);

	Expect(!point.changed && !tile.changed,
		"primary release events should not change accumulator state");
	Expect(frame.eventCount == 0,
		"primary release events should emit no frame events");
}

void TestFrameCarriesBindingContextAndDoesNotMutateInputs()
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state;
	state = Record(state, Event(Control::MoveNorth)).state;
	state = Record(state, Event(Control::Interact)).state;
	iggy::PlayerInputBindingContext2D context;
	context.input.worldInputEnabled = false;
	context.hasCurrentPlayerTile = true;
	context.currentPlayerTile = { 2, 3 };
	context.hasSelectedTargetId = true;
	context.selectedTargetId = iggy::ResourceId { "target:selected" };
	const auto stateBefore = state;
	const auto contextBefore = context;

	const auto frame = BuildFrame(state, context);

	Expect(SameContext(frame.frame.bindingContext, contextBefore),
		"accumulator frame should carry supplied binding context unchanged");
	Expect(state.heldControls.size() == stateBefore.heldControls.size() &&
			state.pendingOneShotEvents.size() ==
				stateBefore.pendingOneShotEvents.size(),
		"accumulator buildFrame should not mutate input state argument");
	Expect(SameContext(context, contextBefore),
		"accumulator buildFrame should not mutate context argument");
	Expect(frame.state.heldControls.size() == 1 &&
			frame.state.pendingOneShotEvents.empty(),
		"accumulator buildFrame should preserve held controls and consume one-shots in returned state");
}

void TestClearReturnsEmptyState()
{
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState state;
	state = Record(state, Event(Control::MoveEast)).state;
	state = Record(state, Event(Control::Wait)).state;

	const auto cleared =
		iggy::runtime::RuntimeGameplayProductInputAccumulator {}.clear(state);

	Expect(cleared.heldControls.empty() &&
			cleared.pendingOneShotEvents.empty(),
		"accumulator clear should return empty state");
	Expect(!state.heldControls.empty() && !state.pendingOneShotEvents.empty(),
		"accumulator clear should not mutate input state argument");
}

void TestHeldMovementFlowsThroughRepeatedFrameRequests()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState playState =
		ReadyState(2);
	iggy::runtime::RuntimeGameplayProductInputAccumulatorState accumulator =
		Record({}, Event(Control::MoveEast)).state;

	iggy::runtime::RuntimeGameplayProductInputAccumulatorFrameInput firstInput;
	firstInput.state = accumulator;
	firstInput.bindingContext =
		iggy::runtime::RuntimeGameplayProductInputContext {}
			.build(playState)
			.bindingContext;
	const auto firstFrame =
		iggy::runtime::RuntimeGameplayProductInputAccumulator {}.buildFrame(
			firstInput);
	accumulator = firstFrame.state;
	const auto firstResult = RunFrameRequest(playState, firstFrame.frame);

	iggy::runtime::RuntimeGameplayProductInputAccumulatorFrameInput secondInput;
	secondInput.state = accumulator;
	secondInput.bindingContext =
		iggy::runtime::RuntimeGameplayProductInputContext {}
			.build(firstResult.state)
			.bindingContext;
	const auto secondFrame =
		iggy::runtime::RuntimeGameplayProductInputAccumulator {}.buildFrame(
			secondInput);
	const auto secondResult = RunFrameRequest(
		firstResult.state,
		secondFrame.frame);

	Expect(firstResult.status == RequestStatus::Stepped &&
			secondResult.status == RequestStatus::Stepped,
		"held movement frame request regression should step twice");
	Expect(firstFrame.eventCount == 1 && secondFrame.eventCount == 1,
		"held movement should emit in both requested frames");
	Expect(iggy::playerTile(firstResult.state.loop.currentState.session.player) ==
			iggy::TileCoord { 1, 0 },
		"first held movement frame should move east once");
	Expect(iggy::playerTile(secondResult.state.loop.currentState.session.player) ==
			iggy::TileCoord { 2, 0 },
		"second held movement frame should move east again without new raw input");
}

} // namespace

int main()
{
	TestMovementPressBuildsHeldPressedEvent();
	TestHeldMovementRepeatsUntilRelease();
	TestDuplicateMovementPressDoesNotDuplicateHeldControl();
	TestReleaseOfNonHeldMovementIsNoOp();
	TestOppositeCardinalsPreservePressOrder();
	TestOneShotPressEmitsOnceAndCanBePressedAgain();
	TestOneShotReleaseIsNoOp();
	TestPrimaryTilePressEmitsOnceWithPayloadPreserved();
	TestPrimaryPointPressEmitsOnceWithPayloadPreserved();
	TestPrimaryReleaseIsNoOp();
	TestFrameCarriesBindingContextAndDoesNotMutateInputs();
	TestClearReturnsEmptyState();
	TestHeldMovementFlowsThroughRepeatedFrameRequests();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
