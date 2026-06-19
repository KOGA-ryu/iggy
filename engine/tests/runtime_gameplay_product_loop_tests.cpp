#include "runtime/RuntimeGameplayProductLoop.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <type_traits>
#include <vector>

#include "core/math/Aabb2.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "servers/physics2d/CollisionShape2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

bool Near(float actual, float expected)
{
	const float delta = actual - expected;
	return delta > -0.001F && delta < 0.001F;
}

bool NearVec(iggy::Vec2 actual, iggy::Vec2 expected)
{
	return Near(actual.x, expected.x) && Near(actual.y, expected.y);
}

bool SameIntent(
	const iggy::PlayerInputIntent2D &actual,
	const iggy::PlayerInputIntent2D &expected)
{
	return actual.type == expected.type
		&& NearVec(actual.worldPoint, expected.worldPoint)
		&& actual.tile.x == expected.tile.x
		&& actual.tile.y == expected.tile.y
		&& actual.targetId == expected.targetId;
}

bool SameContext(
	const iggy::PlayerInputContext2D &actual,
	const iggy::PlayerInputContext2D &expected)
{
	return actual.playerControlEnabled == expected.playerControlEnabled
		&& actual.worldInputEnabled == expected.worldInputEnabled
		&& actual.interactionEnabled == expected.interactionEnabled
		&& actual.cancelEnabled == expected.cancelEnabled;
}

template <typename T, typename = void>
struct HasFinalRowsField : std::false_type {
};

template <typename T>
struct HasFinalRowsField<T, std::void_t<decltype(&T::finalRows)>> :
	std::true_type {
};

template <typename T, typename = void>
struct HasTraceFramesField : std::false_type {
};

template <typename T>
struct HasTraceFramesField<T, std::void_t<decltype(&T::traceFrames)>> :
	std::true_type {
};

template <typename T, typename = void>
struct HasExpectationComparisonField : std::false_type {
};

template <typename T>
struct HasExpectationComparisonField<
	T,
	std::void_t<decltype(&T::expectationComparison)>> : std::true_type {
};

using BuildResult = iggy::runtime::RuntimeGameplayProductLoopBuildResult;
using StepResult = iggy::runtime::RuntimeGameplayProductLoopStepResult;

static_assert(!HasFinalRowsField<BuildResult>::value);
static_assert(!HasTraceFramesField<BuildResult>::value);
static_assert(!HasExpectationComparisonField<BuildResult>::value);
static_assert(!HasFinalRowsField<StepResult>::value);
static_assert(!HasTraceFramesField<StepResult>::value);
static_assert(!HasExpectationComparisonField<StepResult>::value);

const iggy::NpcActorState2D *FindActor(
	const iggy::NpcActorState2DRegistry &registry,
	const iggy::ResourceId &id)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == id)
			return &actor;
	}
	return nullptr;
}

iggy::runtime::RuntimeGameplayProductScenarioLoadResult LoadFixture(
	const char *name)
{
	return iggy::runtime::RuntimeGameplayProductScenarioLoader {}.load(
		FixturePath(name));
}

BuildResult BuildFixture(const char *name)
{
	const iggy::runtime::RuntimeGameplayProductScenarioLoadResult load =
		LoadFixture(name);
	return iggy::runtime::RuntimeGameplayProductLoop {}.build(load);
}

iggy::physics2d::CollisionObject2D Object(
	const char *id,
	iggy::Aabb2 bounds)
{
	return { Id(id), iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(
	std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult result =
		iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(result.built, "product loop collision world should build");
	return result.world;
}

iggy::PlayerInputContext2D WorldInputDisabledContext()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	return context;
}

void TestBuildFromSuccessfulDirectTomlLoad()
{
	const iggy::runtime::RuntimeGameplayProductScenarioLoadResult load =
		LoadFixture("moving_guard_room.toml");

	const BuildResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.build(load);

	Expect(result.status == iggy::runtime::RuntimeGameplayProductLoopStatus::Ready,
		"successful load should build ready product loop state");
	Expect(result.state.loaded, "successful build should mark state loaded");
	Expect(result.state.inputPath == load.inputPath,
		"build should copy input path");
	Expect(result.state.sourcePath == load.sourcePath,
		"build should copy source path");
	Expect(!result.state.hasPackage, "direct TOML build should not mark package");
	Expect(result.state.definition.frames.size() == load.definition.frames.size(),
		"build should copy profile scenario definition");
	Expect(result.state.scenario.frames.size() ==
			load.validation.build.scenario.frames.size(),
		"build should copy lowered runtime scenario");
	Expect(result.state.initialState.session.hasPlayer,
		"build should copy initial state with player");
	Expect(result.state.currentState.session.hasPlayer,
		"build should initialize current state with player");
	Expect(NearVec(
			   result.state.currentState.session.player.position,
			   result.state.initialState.session.player.position),
		"build current state should start at initial state");
	Expect(result.state.nextFrameIndex == 0,
		"build should start at frame index zero");
	Expect(result.load.ok(), "build should preserve nested load result");
}

void TestBuildFromFailedLoadDoesNotProduceLoadedState()
{
	iggy::runtime::RuntimeGameplayProductScenarioLoadResult load;
	load.status =
		iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::InputPathMissing;

	const BuildResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.build(load);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStatus::LoadFailed,
		"failed load should produce LoadFailed");
	Expect(!result.state.loaded,
		"failed load should not produce loaded product loop state");
	Expect(result.load.status == load.status,
		"failed build should preserve nested load result");
}

void TestStepRunsOneFrameWithCallerMoveIntent()
{
	const BuildResult build = BuildFixture("moving_guard_room.toml");
	const std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToTileIntent({ 4, 1 }),
	};
	const std::vector<iggy::PlayerInputIntent2D> intentsBefore = intents;
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.state = build.state;
	input.playerIntents = intents;
	const iggy::runtime::RuntimeGameplayProductLoopState stateBefore =
		input.state;

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"caller move intent should step one frame");
	Expect(result.frameIndex == 0, "first step should report frame index zero");
	Expect(result.state.nextFrameIndex == 1,
		"step should advance next frame index by one");
	Expect(result.frame.acceptedCommandCount == 1,
		"caller move intent should be accepted");
	Expect(iggy::playerTile(result.state.currentState.session.player) ==
			iggy::TileCoord { 4, 1 },
		"caller move intent should update current player state");
	Expect(result.frame.npcMovedCount == 1,
		"step should still run NPC movement from frame template");
	Expect(input.state.nextFrameIndex == stateBefore.nextFrameIndex,
		"step should not mutate input state");
	Expect(NearVec(
			   input.state.currentState.session.player.position,
			   stateBefore.currentState.session.player.position),
		"step should not mutate input current state");
	Expect(input.playerIntents.size() == intentsBefore.size() &&
			SameIntent(input.playerIntents[0], intentsBefore[0]),
		"step should not mutate input intent vector");
}

void TestPlayerInputContextOverrideBlocksCallerMoveIntent()
{
	const BuildResult build = BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.state = build.state;
	input.playerIntents = { iggy::playerMoveToTileIntent({ 4, 1 }) };
	input.hasPlayerInputContextOverride = true;
	input.playerInputContextOverride = WorldInputDisabledContext();
	const iggy::runtime::RuntimeGameplayProductLoopStepInput before = input;
	const iggy::TileCoord startingTile =
		iggy::playerTile(build.state.currentState.session.player);
	const iggy::Vec2 startingPosition =
		build.state.currentState.session.player.position;

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"context override should still step frame");
	Expect(result.frame.blockedIntentCount == 1,
		"world-input-disabled override should block caller move intent");
	Expect(result.frame.acceptedCommandCount == 0,
		"world-input-disabled override should accept no player commands");
	Expect(iggy::playerTile(result.state.currentState.session.player) ==
			startingTile,
		"blocked override should leave player tile unchanged");
	Expect(NearVec(
			   result.state.currentState.session.player.position,
			   startingPosition),
		"blocked override should leave player position unchanged");
	Expect(!result.frame.input.playerFrame.playerInputContext.worldInputEnabled,
		"frame input should expose overridden world input context");
	Expect(input.hasPlayerInputContextOverride ==
			before.hasPlayerInputContextOverride,
		"step should not mutate override presence flag");
	Expect(SameContext(
			   input.playerInputContextOverride,
			   before.playerInputContextOverride),
		"step should not mutate override context");
}

void TestAllEnabledPlayerInputContextOverrideAllowsCallerMoveIntent()
{
	const BuildResult build = BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.state = build.state;
	input.playerIntents = { iggy::playerMoveToTileIntent({ 4, 1 }) };
	input.hasPlayerInputContextOverride = true;
	input.playerInputContextOverride = {};

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"all-enabled context override should step frame");
	Expect(result.frame.acceptedCommandCount == 1,
		"all-enabled context override should accept move intent");
	Expect(result.frame.blockedIntentCount == 0,
		"all-enabled context override should not block move intent");
	Expect(iggy::playerTile(result.state.currentState.session.player) ==
			iggy::TileCoord { 4, 1 },
		"all-enabled context override should update current player state");
	Expect(result.frame.input.playerFrame.playerInputContext.worldInputEnabled,
		"frame input should expose all-enabled override context");
}

void TestCallerIntentsReplaceAuthoredFrameIntents()
{
	const BuildResult build = BuildFixture("player_and_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.state = build.state;
	input.playerIntents = { iggy::playerWaitIntent() };
	const iggy::TileCoord startingTile =
		iggy::playerTile(build.state.currentState.session.player);

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"wait intent should step scripted fixture frame");
	Expect(result.frame.input.playerFrame.playerIntents.size() == 1,
		"product step should use caller intent list");
	if (!result.frame.input.playerFrame.playerIntents.empty()) {
		Expect(result.frame.input.playerFrame.playerIntents[0].type ==
				iggy::PlayerInputIntent2DType::Wait,
			"product step should replace authored move intent with caller wait");
	}
	Expect(iggy::playerTile(result.state.currentState.session.player) ==
			startingTile,
		"caller wait should prevent hidden authored player movement");
	Expect(result.frame.npcMovedCount == 1,
		"replacement should not remove NPC frame controls");
}

void TestMultipleStepsCarryCurrentStateForward()
{
	const BuildResult build = BuildFixture("multi_frame_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput first;
	first.state = build.state;

	const StepResult firstResult =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(first);
	const iggy::NpcActorState2D *firstActor =
		FindActor(firstResult.state.currentState.npcActors, Id("npc:guard"));

	iggy::runtime::RuntimeGameplayProductLoopStepInput second;
	second.state = firstResult.state;
	const StepResult secondResult =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(second);
	const iggy::NpcActorState2D *secondActor =
		FindActor(secondResult.state.currentState.npcActors, Id("npc:guard"));

	Expect(firstResult.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"first multi-frame step should step");
	Expect(secondResult.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"second multi-frame step should step");
	Expect(firstResult.state.nextFrameIndex == 1,
		"first step should consume one frame");
	Expect(secondResult.state.nextFrameIndex == 2,
		"second step should consume second frame");
	Expect(firstActor != nullptr && NearVec(firstActor->position, { 2.5F, 1.5F }),
		"first step should move NPC to first target");
	Expect(secondActor != nullptr &&
			NearVec(secondActor->position, { 3.5F, 1.5F }),
		"second step should carry NPC state forward to second target");
}

void TestExhaustedFramesReturnNoFrameWithoutMutation()
{
	const BuildResult build = BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.state = build.state;
	const StepResult stepped =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input);

	iggy::runtime::RuntimeGameplayProductLoopStepInput exhausted;
	exhausted.state = stepped.state;
	const iggy::runtime::RuntimeGameplayProductLoopState before =
		exhausted.state;
	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(exhausted);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::
				NoFrameAvailable,
		"exhausted product loop should report no frame");
	Expect(result.state.nextFrameIndex == before.nextFrameIndex,
		"exhausted step should not advance frame index");
	Expect(NearVec(
			   result.state.currentState.session.player.position,
			   before.currentState.session.player.position),
		"exhausted step should not mutate current state");
}

void TestFreePlayStepRunsInputWithoutAdvancingFrameCursor()
{
	const BuildResult build = BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput scripted;
	scripted.state = build.state;
	const StepResult scriptedResult =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(scripted);

	iggy::runtime::RuntimeGameplayProductLoopStepInput freePlay;
	freePlay.state = scriptedResult.state;
	freePlay.playerIntents = { iggy::playerMoveToTileIntent({ 4, 1 }) };
	freePlay.hasPlayerInputContextOverride = true;
	freePlay.playerInputContextOverride = {};
	freePlay.allowFreePlayFrameWhenNoFrameAvailable = true;
	const iggy::runtime::RuntimeGameplayProductLoopState before =
		freePlay.state;

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(freePlay);

	Expect(scriptedResult.state.nextFrameIndex ==
			scriptedResult.state.scenario.frames.size(),
		"free-play setup should exhaust authored frames");
	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"free-play product loop should step when explicitly enabled");
	Expect(result.frameIndex == before.nextFrameIndex,
		"free-play step should report exhausted frame cursor");
	Expect(result.state.nextFrameIndex == before.nextFrameIndex,
		"free-play step should not consume or rewind authored frames");
	Expect(result.frame.acceptedCommandCount == 1,
		"free-play step should accept caller movement intent");
	Expect(result.frame.npcMovedCount == 0,
		"free-play step should not replay authored NPC movement");
	Expect(iggy::playerTile(result.state.currentState.session.player) ==
			iggy::TileCoord { 4, 1 },
		"free-play step should update current player state");
	Expect(freePlay.allowFreePlayFrameWhenNoFrameAvailable,
		"free-play step should not mutate caller opt-in flag");
}

void TestNotLoadedStateReturnsNotLoadedWithoutMutation()
{
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.playerIntents = { iggy::playerWaitIntent() };

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::NotLoaded,
		"not-loaded state should report NotLoaded");
	Expect(!result.state.loaded, "not-loaded step should preserve unloaded state");
	Expect(result.state.nextFrameIndex == 0,
		"not-loaded step should not advance frame index");
}

void TestExplicitCollisionWorldOverloadCanBlockMovement()
{
	const BuildResult build = BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.state = build.state;
	input.playerIntents = { iggy::playerMoveToTileIntent({ 4, 1 }) };
	const iggy::Vec2 startingPosition =
		build.state.currentState.session.player.position;
	const iggy::physics2d::CollisionWorld2D blockingWorld =
		World({ Object("wall:block-player-target", { { 4.0F, 1.0F }, { 5.0F, 2.0F } }) });

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input, blockingWorld);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"explicit collision world overload should still step");
	Expect(result.frame.acceptedCommandCount == 1,
		"blocked movement intent should still pass through existing frame step");
	Expect(NearVec(
			   result.state.currentState.session.player.position,
			   startingPosition),
		"explicit collision world should block player movement through existing behavior");
}

void TestExplicitCollisionWorldOverloadPreservesContextOverride()
{
	const BuildResult build = BuildFixture("moving_guard_room.toml");
	iggy::runtime::RuntimeGameplayProductLoopStepInput input;
	input.state = build.state;
	input.playerIntents = { iggy::playerMoveToTileIntent({ 4, 1 }) };
	input.hasPlayerInputContextOverride = true;
	input.playerInputContextOverride = WorldInputDisabledContext();
	const iggy::runtime::RuntimeGameplayProductLoopStepInput before = input;
	const iggy::Vec2 startingPosition =
		build.state.currentState.session.player.position;
	const iggy::physics2d::CollisionWorld2D blockingWorld =
		World({ Object("wall:block-player-target", { { 4.0F, 1.0F }, { 5.0F, 2.0F } }) });

	const StepResult result =
		iggy::runtime::RuntimeGameplayProductLoop {}.step(input, blockingWorld);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductLoopStepStatus::Stepped,
		"explicit collision world override step should still step");
	Expect(result.frame.blockedIntentCount == 1,
		"explicit collision world overload should preserve context gate block");
	Expect(result.frame.acceptedCommandCount == 0,
		"explicit collision world overload should not accept context-blocked intent");
	Expect(!result.frame.input.playerFrame.playerInputContext.worldInputEnabled,
		"explicit collision world frame input should expose override context");
	Expect(NearVec(
			   result.state.currentState.session.player.position,
			   startingPosition),
		"explicit collision world context block should leave player position unchanged");
	Expect(input.hasPlayerInputContextOverride ==
			before.hasPlayerInputContextOverride,
		"explicit collision world step should not mutate override flag");
	Expect(SameContext(
			   input.playerInputContextOverride,
			   before.playerInputContextOverride),
		"explicit collision world step should not mutate override context");
}

} // namespace

int main()
{
	TestBuildFromSuccessfulDirectTomlLoad();
	TestBuildFromFailedLoadDoesNotProduceLoadedState();
	TestStepRunsOneFrameWithCallerMoveIntent();
	TestPlayerInputContextOverrideBlocksCallerMoveIntent();
	TestAllEnabledPlayerInputContextOverrideAllowsCallerMoveIntent();
	TestCallerIntentsReplaceAuthoredFrameIntents();
	TestMultipleStepsCarryCurrentStateForward();
	TestExhaustedFramesReturnNoFrameWithoutMutation();
	TestFreePlayStepRunsInputWithoutAdvancingFrameCursor();
	TestNotLoadedStateReturnsNotLoadedWithoutMutation();
	TestExplicitCollisionWorldOverloadCanBlockMovement();
	TestExplicitCollisionWorldOverloadPreservesContextOverride();

	if (Failures != 0)
		return 1;
	return 0;
}
