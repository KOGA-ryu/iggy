#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "actions/ActionExecutor.hpp"
#include "app/GameLoop.hpp"
#include "app/RuntimeDebugArtifactBundle.hpp"
#include "app/RuntimeDebugArtifactBundleResultBuilder.hpp"
#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugArtifactRootPreparer.hpp"
#include "app/RuntimeDebugArtifactWriter.hpp"
#include "app/RuntimeDebugManifest.hpp"
#include "app/RuntimeDebugManifestContextBuilder.hpp"
#include "app/RuntimeDebugManifestIndexText.hpp"
#include "app/RuntimeDebugManifestPathsText.hpp"
#include "app/RuntimeDebugManifestSections.hpp"
#include "app/RuntimeDebugManifestSetupText.hpp"
#include "app/RuntimeDebugManifestWriteStep.hpp"
#include "app/RuntimeDebugTraceWriteStep.hpp"
#include "app/RuntimeInputContextBuilder.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "app/RuntimeInventoryCommandIntake.hpp"
#include "app/RuntimeInventoryCommandReportRecorder.hpp"
#include "app/RuntimeInventoryScriptReportRecorder.hpp"
#include "app/RuntimeInventoryScriptBatchRunner.hpp"
#include "app/RuntimeInventoryScriptIntake.hpp"
#include "app/RuntimeMovementInputRouter.hpp"
#include "app/RuntimeMovementScriptBatchRunner.hpp"
#include "app/RuntimeMovementScriptReportRecorder.hpp"
#include "app/RuntimeMovementScriptIntake.hpp"
#include "app/RuntimeFrameTrace.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSessionCommandReportRecorder.hpp"
#include "app/RuntimeSessionCommandIntake.hpp"
#include "app/RuntimeSessionInputRouter.hpp"
#include "app/RuntimeSessionModeTogglePolicy.hpp"
#include "app/RuntimeSetupFrameGate.hpp"
#include "app/RuntimeSetupInventoryCommandReportRecorder.hpp"
#include "app/RuntimeSetupRunResultApplier.hpp"
#include "app/RuntimeSetupRunner.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "app/RuntimeStartupScriptIntake.hpp"
#include "app/RuntimeStopMovementInputStep.hpp"
#include "app/RuntimeTraceService.hpp"
#include "combat/CombatEventRecorder.hpp"
#include "combat/CombatResolver.hpp"
#include "combat/CombatSystem.hpp"
#include "commands/CommandDispatcher.hpp"
#include "commands/MovementCommandValidator.hpp"
#include "effects/EffectApplier.hpp"
#include "effects/EffectRecorder.hpp"
#include "effects/EffectRouter.hpp"
#include "enemies/EnemyAttackEntryPolicy.hpp"
#include "enemies/EnemyAttackEventEmitter.hpp"
#include "enemies/EnemyAttackPhaseRunner.hpp"
#include "enemies/EnemyAttackRange.hpp"
#include "enemies/EnemyAttackRestartPolicy.hpp"
#include "enemies/EnemyAttackRunner.hpp"
#include "enemies/EnemyMovement.hpp"
#include "enemies/EnemyMovementReporter.hpp"
#include "enemies/EnemyPursuitBudget.hpp"
#include "enemies/EnemyPursuitEventEmitter.hpp"
#include "enemies/EnemyPursuitStepGate.hpp"
#include "enemies/EnemyPursuitStepPlanner.hpp"
#include "enemies/EnemyPursuitStepper.hpp"
#include "events/EventRecorder.hpp"
#include "files/ByteFileStore.hpp"
#include "files/TextFileStore.hpp"
#include "focus/InputFocus.hpp"
#include "interaction/DestinationActionBuilder.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
#include "inventory/InventoryCommandCodec.hpp"
#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandEventEmitter.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogChecksum.hpp"
#include "inventory/InventoryCommandLogCodec.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandLogFrameCodec.hpp"
#include "inventory/InventoryCommandByteStream.hpp"
#include "inventory/InventoryCommandPacketByteCodec.hpp"
#include "inventory/InventoryCommandPacketListCodec.hpp"
#include "inventory/InventoryCommandPacketValidator.hpp"
#include "inventory/InventoryCommandReplayer.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "inventory/EquipmentService.hpp"
#include "inventory/EquipmentStatsService.hpp"
#include "inventory/InventoryService.hpp"
#include "input/InputEventMatcher.hpp"
#include "input/RawInputSource.hpp"
#include "network/MovementCodec.hpp"
#include "player/ActorStepCommitter.hpp"
#include "player/PlayerAnimationLockGate.hpp"
#include "player/PlayerActionRunner.hpp"
#include "player/PlayerActionGate.hpp"
#include "player/PlayerController.hpp"
#include "player/PlayerMovement.hpp"
#include "player/PlayerPathPlanner.hpp"
#include "player/PlayerPathStepper.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogChecksum.hpp"
#include "replay/CommandLogCodec.hpp"
#include "replay/CommandLogFileStore.hpp"
#include "replay/CommandLogFrameCodec.hpp"
#include "replay/CommandPacketListCodec.hpp"
#include "replay/CommandReplayer.hpp"
#include "replay/MovementScriptRunner.hpp"
#include "replay/MovementScriptSource.hpp"
#include "save/SaveGameService.hpp"
#include "save/SaveSlotService.hpp"
#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotCodec.hpp"
#include "save/SnapshotChecksum.hpp"
#include "save/SnapshotEntityCodec.hpp"
#include "save/SnapshotEnemyCodec.hpp"
#include "save/SnapshotFileStore.hpp"
#include "save/SnapshotFrameCodec.hpp"
#include "save/SnapshotPlayerCodec.hpp"
#include "save/SnapshotReader.hpp"
#include "save/SnapshotSchemaCodec.hpp"
#include "save/SnapshotVectorCodec.hpp"
#include "save/SnapshotWriter.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandApplier.hpp"
#include "session/SessionCommandByteStream.hpp"
#include "session/SessionCommandCodec.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandLog.hpp"
#include "session/SessionCommandLogChecksum.hpp"
#include "session/SessionCommandLogCodec.hpp"
#include "session/SessionCommandLogFileStore.hpp"
#include "session/SessionCommandLogFrameCodec.hpp"
#include "session/SessionCommandPacketByteCodec.hpp"
#include "session/SessionCommandPacketListCodec.hpp"
#include "session/SessionCommandPacketValidator.hpp"
#include "session/SessionCommandReplayer.hpp"
#include "session/SessionEventEmitter.hpp"
#include "session/SessionEventRecorder.hpp"
#include "session/SessionFrameUpdater.hpp"
#include "session/NewGameWorldBuilder.hpp"
#include "session/SessionModeChanger.hpp"
#include "session/SessionModePolicy.hpp"
#include "session/SessionScriptRunner.hpp"
#include "session/SessionWorldSlotLoader.hpp"
#include "session/SessionWorldSlotSaver.hpp"
#include "simulation/SimulationActorUpdater.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationEnemyMovementRunner.hpp"
#include "simulation/SimulationEnemyTargetSelector.hpp"
#include "simulation/SimulationEnemyUpdater.hpp"
#include "simulation/SimulationEffectFinalizer.hpp"
#include "simulation/SimulationEffectPipeline.hpp"
#include "simulation/SimulationFrameEventCapture.hpp"
#include "simulation/SimulationFrameFinalizer.hpp"
#include "simulation/SimulationFramePolicyDescriber.hpp"
#include "simulation/SimulationFrameRunner.hpp"
#include "simulation/SimulationFrameTickRunner.hpp"
#include "simulation/SimulationInventoryFinalizer.hpp"
#include "simulation/SimulationPlayerMovementRunner.hpp"
#include "simulation/SimulationPlayerUpdater.hpp"
#include "simulation/SimulationTargetFinalizer.hpp"
#include "simulation/SimulationTick.hpp"
#include "simulation/SimulationTickPipeline.hpp"
#include "simulation/SimulationTimeStepBuilder.hpp"
#include "simulation/WorldEntityService.hpp"
#include "targeting/Target.hpp"
#include "targeting/TargetRegistry.hpp"
#include "targeting/TargetSynchronizer.hpp"
#include "world/Collision.hpp"
#include "world/PathFinder.hpp"
#include "world/TileMap.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool ContainsLineFragment(const std::vector<std::string> &lines, std::string_view fragment)
{
	for (const std::string &line : lines) {
		if (line.find(fragment) != std::string::npos)
			return true;
	}
	return false;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

dev::Player MakePlayer(dev::Point tile = { 0, 0 })
{
	dev::Player player;
	player.position.tile = tile;
	player.position.future = tile;
	player.position.previous = tile;
	player.position.precise = tile;
	return player;
}

dev::Enemy MakeEnemy(dev::Point tile)
{
	dev::Enemy enemy;
	enemy.id = 1;
	enemy.position.tile = tile;
	enemy.position.future = tile;
	enemy.position.previous = tile;
	enemy.position.precise = tile;
	return enemy;
}

class FixedTargetResolver : public dev::TargetResolver {
public:
	explicit FixedTargetResolver(dev::Target target)
	    : target_(target)
	{
	}

	dev::Target resolveAtTile(dev::Point tile) const override
	{
		dev::Target target = target_;
		target.tile = tile;
		return target;
	}

private:
	dev::Target target_;
};

void TestInventoryFocusBlocksMovement()
{
	dev::FocusState focusState { .owner = dev::InputOwner::Inventory };
	dev::InputFocus focus { focusState };
	dev::PlayerActionContext context;
	dev::PlayerActionGate gate { focus, context };
	dev::Player player = MakePlayer();

	Expect(!gate.canMove(player), "inventory focus should block movement");
	Expect(gate.movementBlockReason(player) == dev::PlayerActionBlockReason::Focus, "inventory focus should explain movement block reason");
}

void TestPlayerActionGateReportsMovementBlockReasons()
{
	dev::FocusState gameplayFocusState;
	dev::InputFocus gameplayFocus { gameplayFocusState };
	dev::PlayerActionContext context;
	dev::Player player = MakePlayer();

	dev::PlayerActionGate openGate { gameplayFocus, context };
	Expect(openGate.canMove(player), "player action gate should allow movement when no constraints block it");
	Expect(openGate.movementBlockReason(player) == dev::PlayerActionBlockReason::None, "player action gate should report no block reason when movement is allowed");

	dev::PlayerActionContext pausedContext { .paused = true };
	dev::PlayerActionGate pausedGate { gameplayFocus, pausedContext };
	Expect(pausedGate.movementBlockReason(player) == dev::PlayerActionBlockReason::Paused, "player action gate should report paused movement block");

	dev::PlayerActionContext animationLockedContext { .animationLocked = true };
	dev::PlayerActionGate animationLockedGate { gameplayFocus, animationLockedContext };
	Expect(animationLockedGate.movementBlockReason(player) == dev::PlayerActionBlockReason::AnimationLocked, "player action gate should report app-level animation lock block");

	dev::Player committedPlayer = MakePlayer();
	committedPlayer.animationLock.active = true;
	committedPlayer.animationLock.elapsedSeconds = 0.25F;
	committedPlayer.animationLock.cancelAfterSeconds = 0.50F;
	Expect(openGate.movementBlockReason(committedPlayer) == dev::PlayerActionBlockReason::AnimationCommitment, "player action gate should report uncancellable animation commitment");

	dev::Player stunnedPlayer = MakePlayer();
	stunnedPlayer.moveState = dev::PlayerMoveState::Stunned;
	Expect(openGate.movementBlockReason(stunnedPlayer) == dev::PlayerActionBlockReason::Stunned, "player action gate should report stunned movement block");
}

void TestInputEventMatcherRecognizesPressedKeysAndPointers()
{
	dev::InputEventMatcher matcher;

	Expect(matcher.pressedKey({ .type = dev::RawInputType::KeyPress, .code = 'S', .pressed = true }, 'S'), "input event matcher should match pressed key code");
	Expect(!matcher.pressedKey({ .type = dev::RawInputType::KeyPress, .code = 'S', .pressed = false }, 'S'), "input event matcher should reject released key");
	Expect(!matcher.pressedKey({ .type = dev::RawInputType::KeyPress, .code = 'A', .pressed = true }, 'S'), "input event matcher should reject different key code");
	Expect(matcher.pressedPointer({ .type = dev::RawInputType::MouseClick, .pressed = true }), "input event matcher should treat pressed mouse click as pointer press");
	Expect(matcher.pressedPointer({ .type = dev::RawInputType::TouchTap, .pressed = true }), "input event matcher should treat pressed touch tap as pointer press");
	Expect(!matcher.pressedPointer({ .type = dev::RawInputType::MouseClick, .pressed = false }), "input event matcher should reject released pointer");
	Expect(!matcher.pressedPointer({ .type = dev::RawInputType::ControllerButton, .pressed = true }), "input event matcher should reject non-pointer buttons");
}

void TestStandGroundCreatesStandAndAct()
{
	dev::FocusState focusState;
	dev::InputFocus focus { focusState };
	dev::PlayerActionContext context;
	dev::PlayerActionGate gate { focus, context };
	dev::Player player = MakePlayer();
	player.movementModifiers.standGround = true;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::InteractionIntent intent = dev::InteractionIntentBuilder {}.build(target, true);
	dev::MovementCommand command = dev::InteractionCommandBuilder {}.build(0, player, intent, gate);

	Expect(command.type == dev::MovementCommandType::StandAndAct, "stand-ground attack should create StandAndAct");
	Expect(command.destinationAction.has_value(), "stand-ground command should keep destination action");
	Expect(command.destinationAction->type == dev::DestinationActionType::Attack, "stand-ground destination action should be attack");
}

void TestDestinationActionBuilderMapsInteractionRanges()
{
	dev::Target item { .type = dev::TargetType::Item, .id = 2, .tile = { 1, 0 } };
	dev::Target npc { .type = dev::TargetType::Npc, .id = 3, .tile = { 2, 0 } };
	dev::DestinationActionBuilder builder;

	dev::DestinationAction pickup = builder.build({ dev::InteractionIntentType::Pickup, item });
	dev::DestinationAction talk = builder.build({ dev::InteractionIntentType::Talk, npc });
	dev::DestinationAction move = builder.build({ dev::InteractionIntentType::Move, dev::Target { .type = dev::TargetType::EmptyTile, .tile = { 3, 0 } } });

	Expect(pickup.type == dev::DestinationActionType::Pickup, "destination action builder should map pickup intent");
	Expect(pickup.rangeTiles == 0, "destination action builder should allow pickups on the destination tile");
	Expect(talk.type == dev::DestinationActionType::Talk && talk.rangeTiles == 1, "destination action builder should map talk range");
	Expect(move.type == dev::DestinationActionType::None, "destination action builder should leave pure movement without action");
}

void TestMoveThenActExecutesAfterPath()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder };
	dev::CommandDispatcher dispatcher { controller };
	dev::PlayerMovement movement { collision };

	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::MovementCommand command {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 0,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dispatcher.dispatch(command);
	Expect(players[0].moveState == dev::PlayerMoveState::Pathing, "MoveThenAct should start pathing");

	movement.update(players, 0.016F);
	Expect(players[0].position.tile == dev::Point { 1, 0 }, "movement should commit next step");
	Expect(players[0].destinationAction.type == dev::DestinationActionType::None, "action should clear after execution");
	Expect(players[0].animationLock.active, "executed attack should apply animation lock");
}

void TestPlayerPathPlannerStartsPathAndEvents()
{
	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	dev::Player player = MakePlayer({ 0, 0 });

	dev::PlayerPathPlanner { map, collision, pathFinder, &events }.walkTo(player, 0, { 1, 0 });

	Expect(player.moveState == dev::PlayerMoveState::Pathing, "player path planner should put walkable destination into pathing state");
	Expect(!player.path.empty(), "player path planner should store planned walk path");
	Expect(events.events().size() == 1 && events.events()[0].type == dev::MovementEventType::PathStarted, "player path planner should emit path started event");
}

void TestMovementCommandValidatorRequiresActionPayloads()
{
	dev::MovementCommandValidator validator;
	dev::MovementCommand walk {
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	};
	dev::MovementCommand moveThenAct = walk;
	moveThenAct.type = dev::MovementCommandType::MoveThenAct;
	dev::MovementCommand standAndAct = walk;
	standAndAct.type = dev::MovementCommandType::StandAndAct;
	standAndAct.destinationAction = dev::DestinationAction {
		dev::DestinationActionType::Attack,
		dev::Target { .type = dev::TargetType::Enemy, .id = 16, .tile = { 1, 0 } },
		1,
	};

	Expect(validator.accepts(walk), "movement command validator should accept walk commands without action payloads");
	Expect(!validator.accepts(moveThenAct), "movement command validator should reject MoveThenAct without action payload");
	Expect(validator.accepts(standAndAct), "movement command validator should accept StandAndAct with action payload");
}

void TestCommandDispatcherRejectsInvalidActionCommand()
{
	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };

	dispatcher.dispatch({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	Expect(players[0].moveState == dev::PlayerMoveState::Idle, "command dispatcher should not route invalid action commands");
	Expect(events.events().size() == 1 && events.events()[0].type == dev::MovementEventType::CommandRejected, "command dispatcher should emit rejected event for invalid action command");
}

void TestDiagonalCornerPolicyBlocksCornerCutting()
{
	dev::TileMap map;
	dev::Collision collision;
	map.setBlocked({ 1, 0 });
	map.setBlocked({ 0, 1 });

	dev::PathFinder pathFinder;
	dev::WalkPath path = pathFinder.findPath({ 0, 0 }, { 1, 1 }, map, collision);

	Expect(path.empty(), "diagonal path should be blocked when both side corners are blocked");
}

void TestActionExecutorWaitsOutOfRange()
{
	dev::Player player = MakePlayer({ 0, 0 });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 4, 0 } };
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionResult result = dev::ActionExecutor {}.update(player);

	Expect(result.type == dev::ActionResultType::OutOfRange, "out-of-range action should wait");
	Expect(player.destinationAction.type == dev::DestinationActionType::Attack, "out-of-range action should stay queued");
}

void TestMoveThenActEventSequence()
{
	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::ActionExecutor actionExecutor { dev::ActionRules {}, &events };
	dev::PlayerMovement movement { collision, actionExecutor, &events };

	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::MovementCommand command {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 0,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dispatcher.dispatch(command);
	movement.update(players, 0.016F);

	const std::vector<dev::MovementEvent> &recorded = events.events();
	Expect(recorded.size() >= 6, "MoveThenAct should emit observable movement/action events");
	Expect(recorded[0].type == dev::MovementEventType::CommandAccepted, "first event should accept command");
	Expect(recorded[1].type == dev::MovementEventType::PathStarted, "second event should start path");
	Expect(recorded[2].type == dev::MovementEventType::StepCommitted, "third event should commit step");
	Expect(recorded[3].type == dev::MovementEventType::DestinationActionReady, "fourth event should make destination action ready");
	Expect(recorded[4].type == dev::MovementEventType::AnimationLocked, "fifth event should lock animation");
	Expect(recorded[5].type == dev::MovementEventType::ActionExecuted, "sixth event should execute action");
}

void TestActorStepCommitterCommitsActorPosition()
{
	dev::ActorPosition position {
		.tile = { 2, 3 },
		.future = { 3, 3 },
		.previous = { 1, 3 },
		.precise = { 2, 3 },
	};

	dev::ActorStepCommitter {}.commit(position, { 4, 5 });

	Expect(position.previous == dev::Point { 2, 3 }, "actor step committer should preserve old tile as previous");
	Expect(position.tile == dev::Point { 4, 5 }, "actor step committer should update tile to committed step");
	Expect(position.future == dev::Point { 4, 5 }, "actor step committer should update future to committed step");
	Expect(position.precise == dev::Point { 4, 5 }, "actor step committer should update precise position to committed step");
}

void TestPlayerPathStepperReportsActionReady()
{
	dev::EventRecorder events;
	dev::Collision collision;
	dev::Player player = MakePlayer({ 0, 0 });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 14, .tile = { 1, 0 } };
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Pathing;
	player.path.pushStep({ 1, 0 });

	const dev::PlayerPathStepResult result = dev::PlayerPathStepper { collision, &events }.step(player);

	Expect(result.consumedStep, "player path stepper should consume a queued path step");
	Expect(result.actionReady, "player path stepper should report destination action readiness");
	Expect(player.position.tile == dev::Point { 1, 0 }, "player path stepper should commit the next tile");
	Expect(player.moveState == dev::PlayerMoveState::Acting, "player path stepper should leave arrived action in acting state");
	Expect(events.events().size() == 2, "player path stepper should emit step and action-ready events");
	Expect(events.events().size() == 2 && events.events()[0].type == dev::MovementEventType::StepCommitted, "player path stepper should emit step committed first");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::MovementEventType::DestinationActionReady, "player path stepper should emit action ready second");
}

void TestPlayerAnimationLockGateBlocksUntilCancelWindow()
{
	dev::EventRecorder events;
	dev::Player player = MakePlayer({ 2, 0 });
	player.animationLock.active = true;
	player.animationLock.cancelAfterSeconds = 0.50F;
	dev::PlayerAnimationLockGate gate { &events };

	Expect(!gate.advance(player, 0.25F), "animation lock gate should block before cancel window");
	Expect(player.animationLock.active, "animation lock should remain active before cancel window");
	Expect(events.events().empty(), "animation lock gate should not emit unlock before cancel window");

	Expect(gate.advance(player, 0.25F), "animation lock gate should open once cancel window is reached");
	Expect(!player.animationLock.active, "animation lock gate should clear active lock at cancel window");
	Expect(events.events().size() == 1 && events.events()[0].type == dev::MovementEventType::AnimationUnlocked, "animation lock gate should emit animation unlock event");
}

void TestPlayerActionRunnerExecutesReadyDestinationAction()
{
	dev::EventRecorder events;
	dev::Player player = MakePlayer({ 1, 0 });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 15, .tile = { 1, 0 } };
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::PlayerActionRunner { dev::ActionExecutor { dev::ActionRules {}, &events } }.run(player, 0);

	Expect(player.destinationAction.type == dev::DestinationActionType::None, "player action runner should clear executed destination action");
	Expect(player.moveState == dev::PlayerMoveState::Idle, "player action runner should settle executed action back to idle");
	Expect(player.animationLock.active, "player action runner should preserve action animation commitment");
	Expect(events.events().size() >= 2 && events.events()[0].type == dev::MovementEventType::AnimationLocked, "player action runner should emit animation lock through action executor");
	Expect(events.events().size() >= 2 && events.events()[1].type == dev::MovementEventType::ActionExecuted, "player action runner should emit action executed through action executor");
}

void TestCommandReplayProducesSameEventSequence()
{
	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::MovementCommand command {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 0,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dev::CommandLog log;
	log.record(command);
	Expect(!log.empty(), "command log should record command");

	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::CommandReplayer replayer { dispatcher };
	dev::ActionExecutor actionExecutor { dev::ActionRules {}, &events };
	dev::PlayerMovement movement { collision, actionExecutor, &events };

	dev::CommandReplayReport replayReport = replayer.replay(log);
	movement.update(players, 0.016F);

	const std::vector<dev::MovementEvent> &recorded = events.events();
	Expect(replayReport.results.size() == 1, "replayed command should report one dispatch result");
	Expect(replayReport.acceptedCount() == 1, "replayed command should count accepted dispatches");
	Expect(replayReport.rejectedCount() == 0, "replayed command should count no rejected dispatches");
	Expect(replayReport.allAccepted(), "replayed command should report all accepted");
	Expect(replayReport.results.size() == 1 && replayReport.results[0].type == dev::MovementCommandDispatchResultType::Accepted, "replayed command should report accepted dispatch");
	Expect(replayReport.results.size() == 1 && replayReport.results[0].command.type == dev::MovementCommandType::MoveThenAct, "replayed command should preserve dispatched command");
	Expect(recorded.size() >= 6, "replayed command should produce movement/action events");
	Expect(recorded[0].type == dev::MovementEventType::CommandAccepted, "replay should accept command");
	Expect(recorded[1].type == dev::MovementEventType::PathStarted, "replay should start path");
	Expect(recorded[2].type == dev::MovementEventType::StepCommitted, "replay should commit step");
	Expect(recorded[5].type == dev::MovementEventType::ActionExecuted, "replay should execute action");
}

void TestCommandReplayReportsRejectedCommands()
{
	dev::MovementCommand invalid {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 0,
		.destination = { 1, 0 },
		.destinationAction = std::nullopt,
	};
	dev::CommandLog log;
	log.record(invalid);

	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::CommandReplayer replayer { dispatcher };

	dev::CommandReplayReport replayReport = replayer.replay(log);

	Expect(replayReport.results.size() == 1, "rejected replay command should report one dispatch result");
	Expect(replayReport.acceptedCount() == 0, "rejected replay command should count no accepted dispatches");
	Expect(replayReport.rejectedCount() == 1, "rejected replay command should count rejected dispatches");
	Expect(!replayReport.allAccepted(), "rejected replay command should report not all accepted");
	Expect(replayReport.results.size() == 1 && replayReport.results[0].type == dev::MovementCommandDispatchResultType::Rejected, "rejected replay command should report rejected dispatch");
	Expect(replayReport.results.size() == 1 && replayReport.results[0].command.type == dev::MovementCommandType::MoveThenAct, "rejected replay command should preserve rejected command");
	Expect(players[0].moveState == dev::PlayerMoveState::Idle, "rejected replay command should not move player");
	Expect(events.events().size() == 1 && events.events()[0].type == dev::MovementEventType::CommandRejected, "rejected replay command should still emit rejection event");
}

void TestMovementCodecRoundTrip()
{
	dev::Target target { .type = dev::TargetType::Enemy, .id = 42, .tile = { 10, 6 } };
	dev::MovementCommand original {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 2,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dev::MovementCodec codec;
	dev::MovementPacket packet = codec.toPacket(original);
	dev::PacketBytes bytes = codec.encode(packet);
	std::optional<dev::MovementPacket> decodedPacket = codec.decode(bytes);
	Expect(decodedPacket.has_value(), "encoded packet should decode");

	std::optional<dev::MovementCommand> decoded = decodedPacket.has_value()
	    ? codec.fromPacket(*decodedPacket)
	    : std::nullopt;
	Expect(decoded.has_value(), "decoded packet should become command");
	if (!decoded.has_value())
		return;

	Expect(decoded->type == original.type, "codec should preserve command type");
	Expect(decoded->playerId == original.playerId, "codec should preserve player id");
	Expect(decoded->destination == original.destination, "codec should preserve destination");
	Expect(decoded->destinationAction.has_value(), "codec should preserve destination action");
	Expect(decoded->destinationAction->type == dev::DestinationActionType::Attack, "codec should preserve action type");
	Expect(decoded->destinationAction->target.type == dev::TargetType::Enemy, "codec should preserve target type");
	Expect(decoded->destinationAction->target.id == 42, "codec should preserve target id");
	Expect(decoded->destinationAction->target.tile == target.tile, "codec should preserve target tile");
	Expect(decoded->destinationAction->rangeTiles == 1, "codec should preserve range");
}

void TestCommandLogCodecRoundTripsAndReplays()
{
	dev::Target target { .type = dev::TargetType::Enemy, .id = 43, .tile = { 2, 0 } };
	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	log.record({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = target.tile,
	    .destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	});

	dev::CommandLogCodec codec;
	dev::CommandLogBytes bytes = codec.encode(log);
	std::optional<dev::CommandLog> decoded = codec.decode(bytes);
	Expect(decoded.has_value(), "movement command log codec should decode its own bytes");
	Expect(decoded.has_value() && decoded->commands().size() == 2, "movement command log codec should preserve command count");
	Expect(decoded.has_value() && decoded->commands()[0].destination == dev::Point { 1, 0 }, "movement command log codec should preserve move destination");
	Expect(decoded.has_value() && decoded->commands()[1].destinationAction.has_value(), "movement command log codec should preserve destination action");
	Expect(decoded.has_value() && decoded->commands()[1].destinationAction->target.id == 43, "movement command log codec should preserve action target");

	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::CommandReplayer replayer { dispatcher };
	dev::ActionExecutor actionExecutor { dev::ActionRules {}, &events };
	dev::PlayerMovement movement { collision, actionExecutor, &events };
	dev::CommandReplayReport report = decoded.has_value()
	    ? replayer.replay(*decoded)
	    : dev::CommandReplayReport {};
	movement.update(players, 0.016F);
	movement.update(players, 0.016F);

	Expect(report.results.size() == 2, "decoded movement command log should replay every command");
	Expect(report.acceptedCount() == 2, "decoded movement command log should replay accepted commands");
	Expect(report.allAccepted(), "decoded movement command log should report all accepted");
	Expect(players[0].position.tile == dev::Point { 2, 0 }, "decoded movement command log should reproduce player movement");
}

void TestCommandLogCodecRejectsInvalidBytes()
{
	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::CommandLogCodec codec;
	dev::CommandLogBytes bytes = codec.encode(log);

	dev::CommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "movement command log codec should reject bad magic");

	dev::CommandLogBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 2;
	dev::CommandLogChecksum {}.appendTo(badVersion);
	Expect(!codec.decode(badVersion).has_value(), "movement command log codec should reject bad version");

	dev::CommandLogBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "movement command log codec should reject truncated bytes");

	dev::CommandLogBytes corrupted = bytes;
	corrupted[12] ^= 0x01U;
	Expect(!codec.decode(corrupted).has_value(), "movement command log codec should reject checksum mismatch");
}

void TestCommandLogChecksumValidatesTrailingChecksum()
{
	dev::CommandLogBytes bytes { 1, 2, 3, 4 };
	dev::CommandLogChecksum checksum;
	const uint32_t expected = checksum.compute(bytes, bytes.size());

	checksum.appendTo(bytes);

	Expect(bytes.size() == 8, "movement command log checksum should append four checksum bytes");
	Expect(checksum.hasValidTrailingChecksum(bytes, 4), "movement command log checksum should validate appended checksum");
	Expect(expected == checksum.compute(bytes, 4), "movement command log checksum should compute payload hash only");

	bytes[0] ^= 0xFFU;
	Expect(!checksum.hasValidTrailingChecksum(bytes, 4), "movement command log checksum should reject mutated payload");
}

void TestCommandPacketListCodecFramesPacketBytes()
{
	dev::MovementCommand command {
		.type = dev::MovementCommandType::WalkTo,
		.playerId = 1,
		.destination = { 2, 3 },
	};
	dev::MovementCodec movementCodec;
	dev::PacketBytes packetBytes = movementCodec.encode(movementCodec.toPacket(command));

	dev::CommandLogBytes bytes = dev::CommandPacketListCodec {}.encode({ packetBytes, packetBytes });
	std::optional<std::vector<dev::PacketBytes>> decoded = dev::CommandPacketListCodec {}.decode(bytes);

	Expect(decoded.has_value(), "movement command packet list codec should decode encoded packet lists");
	Expect(decoded.has_value() && decoded->size() == 2, "movement command packet list codec should preserve packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packetBytes, "movement command packet list codec should preserve first packet bytes");
	Expect(decoded.has_value() && (*decoded)[1] == packetBytes, "movement command packet list codec should preserve second packet bytes");
}

void TestCommandPacketListCodecRejectsInvalidSizes()
{
	dev::CommandLogBytes missingCount { 1, 2 };
	dev::CommandLogBytes wrongSize {
		1, 0, 0, 0,
		1, 2, 3,
	};

	dev::CommandPacketListCodec codec;
	Expect(!codec.decode(missingCount).has_value(), "movement command packet list codec should reject missing command count");
	Expect(!codec.decode(wrongSize).has_value(), "movement command packet list codec should reject packet lists with invalid size");
}

void TestCommandLogFrameCodecFramesPacketBytes()
{
	dev::MovementCodec movementCodec;
	std::vector<dev::PacketBytes> packets {
		movementCodec.encode(movementCodec.toPacket({
		    .type = dev::MovementCommandType::WalkTo,
		    .playerId = 0,
		    .destination = { 1, 0 },
		})),
		movementCodec.encode(movementCodec.toPacket({
		    .type = dev::MovementCommandType::Stop,
		    .playerId = 0,
		    .destination = { 1, 0 },
		})),
	};

	dev::CommandLogFrameCodec frameCodec;
	dev::CommandLogBytes bytes = frameCodec.encode(packets);
	std::optional<std::vector<dev::PacketBytes>> decoded = frameCodec.decode(bytes);

	Expect(bytes.size() == 68, "movement command log frame codec should write header, packets, and checksum");
	Expect(bytes.size() == 68 && bytes[0] == 'I' && bytes[1] == 'M' && bytes[2] == 'C' && bytes[3] == 'L', "movement command log frame codec should write magic");
	Expect(bytes.size() == 68 && bytes[4] == 1 && bytes[8] == 2, "movement command log frame codec should write version and command count");
	Expect(decoded.has_value() && decoded->size() == 2, "movement command log frame codec should restore packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packets[0], "movement command log frame codec should preserve first packet");
	Expect(decoded.has_value() && (*decoded)[1] == packets[1], "movement command log frame codec should preserve second packet");
}

void TestCommandLogFrameCodecRejectsInvalidFrames()
{
	dev::MovementCodec movementCodec;
	std::vector<dev::PacketBytes> packets {
		movementCodec.encode(movementCodec.toPacket({
		    .type = dev::MovementCommandType::WalkTo,
		    .playerId = 0,
		    .destination = { 1, 0 },
		})),
	};
	dev::CommandLogFrameCodec frameCodec;
	dev::CommandLogBytes bytes = frameCodec.encode(packets);

	dev::CommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!frameCodec.decode(badMagic).has_value(), "movement command log frame codec should reject checksum-protected bad magic");

	dev::CommandLogBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 2;
	dev::CommandLogChecksum {}.appendTo(badVersion);
	Expect(!frameCodec.decode(badVersion).has_value(), "movement command log frame codec should reject unsupported version");

	dev::CommandLogBytes wrongCount = bytes;
	wrongCount.resize(wrongCount.size() - 4U);
	wrongCount[8] = 2;
	dev::CommandLogChecksum {}.appendTo(wrongCount);
	Expect(!frameCodec.decode(wrongCount).has_value(), "movement command log frame codec should reject payload size mismatch");
}

void TestCommandLogFileStoreSavesLoadsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_movement_log_file_store_replay_test";
	const std::filesystem::path path = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	log.record({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::CommandLogFileStore store;
	Expect(store.save(path, log), "movement command log file store should save log bytes");
	std::optional<dev::CommandLog> loaded = store.load(path);

	Expect(loaded.has_value(), "movement command log file store should load saved log");
	Expect(loaded.has_value() && loaded->commands().size() == 2, "movement command log file store should preserve command count");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "movement command log file store should remove temp file after save");

	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::CommandReplayer replayer { dispatcher };
	dev::CommandReplayReport report = loaded.has_value()
	    ? replayer.replay(*loaded)
	    : dev::CommandReplayReport {};

	Expect(report.results.size() == 2, "loaded movement command log should replay");
	Expect(report.acceptedCount() == 2, "loaded movement command log should preserve accepted dispatches");
	std::size_t acceptedCommands = 0;
	for (const dev::MovementEvent &event : events.events()) {
		if (event.type == dev::MovementEventType::CommandAccepted)
			++acceptedCommands;
	}
	Expect(acceptedCommands == 2, "loaded movement command log replay should emit accepted command events");

	std::filesystem::remove_all(root);
}

void TestCommandLogFileStoreRejectsCorruptAndMissingFiles()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_movement_log_file_store_corrupt_test";
	const std::filesystem::path missingPath = root / "missing.imcl";
	const std::filesystem::path corruptPath = root / "corrupt.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	{
		std::ofstream output { corruptPath, std::ios::binary | std::ios::trunc };
		output << "not a movement command log";
	}

	dev::CommandLogFileStore store;
	Expect(!store.load(missingPath).has_value(), "movement command log file store should reject missing file");
	Expect(!store.load(corruptPath).has_value(), "movement command log file store should reject corrupt file");

	std::filesystem::remove_all(root);
}

void TestMovementScriptRunnerRunsSavedMovementScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_movement_script_runner_test";
	const std::filesystem::path path = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	log.record({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore store;
	Expect(store.save(path, log), "movement script runner test should save movement script");

	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::MovementScriptRunner runner { dispatcher };

	const dev::MovementScriptRunResult result = runner.run(path);

	Expect(result.status == dev::MovementScriptRunStatus::Completed, "movement script runner should report completed scripts");
	Expect(result.replayReport.results.size() == 2, "movement script runner should replay every loaded command");
	Expect(result.replayReport.acceptedCount() == 2, "movement script runner should report accepted commands");
	Expect(result.replayReport.allAccepted(), "movement script runner should report all accepted commands");
	Expect(players[0].moveState == dev::PlayerMoveState::Idle, "movement script runner should let stop command settle player state");

	std::filesystem::remove_all(root);
}

void TestMovementScriptRunnerDistinguishesLoadFailureFromCommandRejection()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_movement_script_runner_failure_test";
	const std::filesystem::path missingPath = root / "missing.imcl";
	const std::filesystem::path rejectedPath = root / "rejected.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog rejectedLog;
	rejectedLog.record({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});
	dev::CommandLogFileStore store;
	Expect(store.save(rejectedPath, rejectedLog), "movement script runner failure test should save rejected script");

	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::MovementScriptRunner runner { dispatcher };

	const dev::MovementScriptRunResult missing = runner.run(missingPath);
	const dev::MovementScriptRunResult rejected = runner.run(rejectedPath);

	Expect(missing.status == dev::MovementScriptRunStatus::LoadFailed, "movement script runner should report load failure for missing scripts");
	Expect(missing.replayReport.results.empty(), "movement script runner should not replay commands after load failure");
	Expect(rejected.status == dev::MovementScriptRunStatus::Completed, "movement script runner should complete loaded scripts even when commands reject");
	Expect(rejected.replayReport.results.size() == 1, "movement script runner should report rejected command result");
	Expect(rejected.replayReport.rejectedCount() == 1, "movement script runner should keep command rejection separate from load failure");
	Expect(!rejected.replayReport.allAccepted(), "movement script runner should expose rejected loaded scripts");

	std::filesystem::remove_all(root);
}

void TestEnemyPursuitStepPlannerChoosesNextTileTowardTarget()
{
	dev::EnemyPursuitStepPlanner planner;

	Expect(planner.nextStepToward({ 0, 0 }, { 3, 2 }) == dev::Point { 1, 1 }, "enemy pursuit planner should step diagonally toward target");
	Expect(planner.nextStepToward({ 4, 2 }, { 1, 2 }) == dev::Point { 3, 2 }, "enemy pursuit planner should step horizontally toward target");
	Expect(planner.nextStepToward({ 4, 5 }, { 4, 5 }) == dev::Point { 4, 5 }, "enemy pursuit planner should stay when already at target");
}

void TestEnemyPursuitStepGateRequiresWalkableUnblockedTile()
{
	dev::TileMap map;
	dev::Collision collision;
	map.setBlocked({ 2, 0 });
	collision.setBlocked({ 3, 0 });

	dev::EnemyPursuitStepGate gate { map, collision };

	Expect(gate.canEnter({ 1, 0 }), "enemy pursuit step gate should allow walkable unblocked tiles");
	Expect(!gate.canEnter({ 2, 0 }), "enemy pursuit step gate should reject map-blocked tiles");
	Expect(!gate.canEnter({ 3, 0 }), "enemy pursuit step gate should reject collision-blocked tiles");
	Expect(!gate.canEnter({ -1, 0 }), "enemy pursuit step gate should reject out-of-bounds tiles");
}

void TestEnemyAttackRangeUsesEnemyTuning()
{
	dev::EnemyAttackRange range;
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.tuning.attackRangeTiles = 1;
	dev::Player diagonalTarget = MakePlayer({ 1, 1 });
	dev::Player distantTarget = MakePlayer({ 2, 0 });

	Expect(range.contains(enemy, diagonalTarget), "enemy attack range should include diagonal tiles within Chebyshev range");
	Expect(!range.contains(enemy, distantTarget), "enemy attack range should reject targets beyond tuning range");

	enemy.tuning.attackRangeTiles = 2;
	Expect(range.contains(enemy, distantTarget), "enemy attack range should honor larger tuning ranges");
}

void TestEnemyAttackEntryPolicyUsesRange()
{
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.tuning.attackRangeTiles = 1;
	dev::Player closeTarget = MakePlayer({ 1, 0 });
	dev::Player farTarget = MakePlayer({ 3, 0 });
	dev::EnemyAttackEntryPolicy policy;

	Expect(policy.shouldStartWindup(enemy, closeTarget), "enemy attack entry policy should start windup when target is in range");
	Expect(!policy.shouldStartWindup(enemy, farTarget), "enemy attack entry policy should wait when target is out of range");
}

void TestEnemyPursuitBudgetUsesMaxStepsPerTick()
{
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.tuning.maxStepsPerTick = 2;
	dev::EnemyPursuitBudget budget;

	Expect(budget.canSpendStep(enemy, 0), "enemy pursuit budget should allow first pursuit step");
	Expect(budget.canSpendStep(enemy, 1), "enemy pursuit budget should allow steps below maxStepsPerTick");
	Expect(!budget.canSpendStep(enemy, 2), "enemy pursuit budget should stop at maxStepsPerTick");
	Expect(!budget.canSpendStep(enemy, -1), "enemy pursuit budget should reject invalid spent step counts");
}

void TestEnemyPursuitObeysStepBudget()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::Player player = MakePlayer({ 4, 0 });
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].tuning.maxStepsPerTick = 1;

	dev::EnemyMovement movement { map, collision };
	movement.update(enemies, player, 0.016F);

	Expect(enemies[0].position.tile == dev::Point { 1, 0 }, "enemy should move only one step toward player");
	Expect(enemies[0].moveState == dev::EnemyMoveState::Pursuing, "enemy should be pursuing after constrained movement");
}

void TestEnemyPursuitStepperStopsAtAttackRange()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::EnemyPursuitStepper pursuit { map, collision };
	dev::Player player = MakePlayer({ 4, 0 });
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.tuning.maxStepsPerTick = 5;
	enemy.tuning.attackRangeTiles = 1;

	const dev::EnemyPursuitResult result = pursuit.pursue(enemy, player);

	Expect(enemy.position.tile == dev::Point { 3, 0 }, "enemy pursuit stepper should stop once attack range is reached");
	Expect(enemy.moveState == dev::EnemyMoveState::Pursuing, "enemy pursuit stepper should leave windup start for the next enemy update");
	Expect(result.stopReason == dev::EnemyPursuitStopReason::AttackRangeReached, "enemy pursuit result should report attack range stop");
	Expect(result.stepsCommitted == 3, "enemy pursuit result should count committed pursuit steps");
}

void TestEnemyPursuitStepperReportsBudgetAndBlockedStops()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::EnemyPursuitStepper pursuit { map, collision };
	dev::Player player = MakePlayer({ 4, 0 });
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.tuning.maxStepsPerTick = 1;
	enemy.tuning.attackRangeTiles = 0;

	const dev::EnemyPursuitResult budgetResult = pursuit.pursue(enemy, player);

	Expect(budgetResult.stopReason == dev::EnemyPursuitStopReason::BudgetSpent, "enemy pursuit result should report budget stop");
	Expect(budgetResult.stepsCommitted == 1, "enemy pursuit result should count budgeted steps");

	collision.setBlocked({ 2, 0 });
	const dev::EnemyPursuitResult blockedResult = pursuit.pursue(enemy, player);

	Expect(blockedResult.stopReason == dev::EnemyPursuitStopReason::Blocked, "enemy pursuit result should report blocked stop");
	Expect(blockedResult.stepsCommitted == 0, "enemy pursuit result should not count rejected blocked steps");
	Expect(enemy.position.tile == dev::Point { 1, 0 }, "blocked pursuit step should leave enemy on last committed tile");
}

void TestEnemyPursuitStepperReportsAlreadyAtTarget()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::EnemyPursuitStepper pursuit { map, collision };
	dev::Player player = MakePlayer({ 2, 0 });
	dev::Enemy enemy = MakeEnemy({ 2, 0 });

	const dev::EnemyPursuitResult result = pursuit.pursue(enemy, player);

	Expect(result.stopReason == dev::EnemyPursuitStopReason::AlreadyAtTarget, "enemy pursuit result should report already-at-target stop");
	Expect(result.stepsCommitted == 0, "enemy pursuit result should not count steps when already at target");
}

void TestEnemyPursuitEventEmitterRecordsStopReason()
{
	dev::EventRecorder events;
	dev::Enemy enemy = MakeEnemy({ 3, 0 });
	enemy.id = 77;
	dev::EnemyPursuitResult result {
		.stepsCommitted = 2,
		.stopReason = dev::EnemyPursuitStopReason::Blocked,
	};

	dev::EnemyPursuitEventEmitter { &events }.emit(enemy, result);

	Expect(events.events().size() == 1, "enemy pursuit event emitter should emit one movement event");
	if (events.events().empty())
		return;

	const dev::MovementEvent &event = events.events()[0];
	Expect(event.type == dev::MovementEventType::EnemyPursuitStopped, "enemy pursuit event should report pursuit stop type");
	Expect(event.tile == dev::Point { 3, 0 }, "enemy pursuit event should report enemy tile");
	Expect(event.enemyId.has_value() && *event.enemyId == 77, "enemy pursuit event should report enemy id");
	Expect(event.enemyPursuitStopReason == dev::EnemyPursuitStopReason::Blocked, "enemy pursuit event should report stop reason");
	Expect(event.enemyPursuitStepsCommitted == 2, "enemy pursuit event should report committed step count");
}

void TestEnemyMovementEmitsPursuitResult()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::EventRecorder events;
	dev::Player player = MakePlayer({ 4, 0 });
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].id = 78;
	enemies[0].tuning.maxStepsPerTick = 1;
	enemies[0].tuning.attackRangeTiles = 0;

	dev::EnemyMovement { map, collision, &events }.update(enemies, player, 0.016F);

	Expect(!events.events().empty(), "enemy movement should emit pursuit result events");
	if (events.events().empty())
		return;

	const dev::MovementEvent &event = events.events()[0];
	Expect(event.type == dev::MovementEventType::EnemyPursuitStopped, "enemy movement should emit enemy pursuit stopped event");
	Expect(event.enemyId == 78, "enemy movement pursuit event should include enemy id");
	Expect(event.enemyPursuitStopReason == dev::EnemyPursuitStopReason::BudgetSpent, "enemy movement pursuit event should report stop reason");
	Expect(event.enemyPursuitStepsCommitted == 1, "enemy movement pursuit event should include committed step count");
}

void TestEnemyAttackWindupAndRecovery()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::Player player = MakePlayer({ 1, 0 });
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].tuning.attackRangeTiles = 1;
	enemies[0].tuning.attackWindupSeconds = 0.25F;
	enemies[0].tuning.attackRecoverySeconds = 0.50F;

	dev::EnemyMovement movement { map, collision };
	movement.update(enemies, player, 0.016F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Attacking, "enemy in range should enter attack windup");

	movement.update(enemies, player, 0.25F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Recovering, "enemy should enter recovery after windup");

	movement.update(enemies, player, 0.25F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Recovering, "enemy should remain in recovery until recovery duration completes");
}

void TestEnemyAttackRunnerConsumesWindupAndRecovery()
{
	dev::Player player = MakePlayer({ 1, 0 });
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.tuning.attackRangeTiles = 1;
	enemy.tuning.attackWindupSeconds = 0.25F;
	enemy.tuning.attackRecoverySeconds = 0.50F;
	dev::EnemyAttackRunner attacks;

	const dev::EnemyAttackResult started = attacks.update(enemy, player, 0.016F);
	Expect(started.consumedFrame, "enemy attack runner should consume frame when target is in range");
	Expect(started.transition == dev::EnemyAttackTransition::WindupStarted, "enemy attack runner should report windup start");
	Expect(enemy.moveState == dev::EnemyMoveState::Attacking, "enemy attack runner should enter attack windup");

	const dev::EnemyAttackResult completedWindup = attacks.update(enemy, player, 0.25F);
	Expect(completedWindup.consumedFrame, "enemy attack runner should consume windup completion frame");
	Expect(completedWindup.transition == dev::EnemyAttackTransition::WindupCompleted, "enemy attack runner should report windup completion");
	Expect(enemy.moveState == dev::EnemyMoveState::Recovering, "enemy attack runner should enter recovery after windup");

	const dev::EnemyAttackResult recovering = attacks.update(enemy, player, 0.25F);
	Expect(recovering.consumedFrame, "enemy attack runner should consume incomplete recovery frame");
	Expect(recovering.transition == dev::EnemyAttackTransition::None, "enemy attack runner should not report transition while recovery is still ticking");
	Expect(enemy.moveState == dev::EnemyMoveState::Recovering, "enemy attack runner should stay recovering until recovery completes");

	const dev::EnemyAttackResult restarted = attacks.update(enemy, player, 0.25F);
	Expect(restarted.consumedFrame, "enemy attack runner should restart windup when recovery completes in range");
	Expect(restarted.transition == dev::EnemyAttackTransition::RecoveryCompletedAndWindupStarted, "enemy attack runner should report recovery completion and windup restart");
	Expect(enemy.moveState == dev::EnemyMoveState::Attacking, "enemy attack runner should restart attack after recovery if target remains in range");
}

void TestEnemyAttackRunnerReportsRecoveryCompletedOutOfRange()
{
	dev::Player player = MakePlayer({ 4, 0 });
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.moveState = dev::EnemyMoveState::Recovering;
	enemy.tuning.attackRangeTiles = 1;
	enemy.tuning.attackRecoverySeconds = 0.25F;
	dev::EnemyAttackRunner attacks;

	const dev::EnemyAttackResult result = attacks.update(enemy, player, 0.25F);

	Expect(!result.consumedFrame, "enemy attack runner should release frame after recovery if target is out of range");
	Expect(result.transition == dev::EnemyAttackTransition::RecoveryCompleted, "enemy attack runner should report recovery completion out of range");
	Expect(enemy.moveState == dev::EnemyMoveState::Recovering, "enemy attack runner should leave movement layer to choose next state after recovery");
}

void TestEnemyAttackPhaseRunnerAdvancesWindupAndRecovery()
{
	dev::EnemyAttackPhaseRunner phases;
	dev::Player player = MakePlayer({ 1, 0 });
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.moveState = dev::EnemyMoveState::Attacking;
	enemy.tuning.attackWindupSeconds = 0.25F;
	enemy.tuning.attackRecoverySeconds = 0.50F;

	const dev::EnemyAttackResult charging = phases.advanceWindup(enemy, player, 0.10F, nullptr);
	Expect(charging.consumedFrame, "enemy attack phase runner should consume incomplete windup");
	Expect(charging.transition == dev::EnemyAttackTransition::None, "enemy attack phase runner should not transition during incomplete windup");
	Expect(enemy.moveState == dev::EnemyMoveState::Attacking, "enemy attack phase runner should keep incomplete windup attacking");

	const dev::EnemyAttackResult completedWindup = phases.advanceWindup(enemy, player, 0.15F, nullptr);
	Expect(completedWindup.consumedFrame, "enemy attack phase runner should consume completed windup");
	Expect(completedWindup.transition == dev::EnemyAttackTransition::WindupCompleted, "enemy attack phase runner should report windup completion");
	Expect(enemy.moveState == dev::EnemyMoveState::Recovering, "enemy attack phase runner should enter recovery after windup");
	Expect(Near(enemy.stateTimerSeconds, 0.0F), "enemy attack phase runner should reset timer after windup completion");

	const dev::EnemyAttackResult recovering = phases.advanceRecovery(enemy, 0.25F);
	Expect(recovering.consumedFrame, "enemy attack phase runner should consume incomplete recovery");
	Expect(recovering.transition == dev::EnemyAttackTransition::None, "enemy attack phase runner should not transition during incomplete recovery");

	const dev::EnemyAttackResult completedRecovery = phases.advanceRecovery(enemy, 0.25F);
	Expect(!completedRecovery.consumedFrame, "enemy attack phase runner should release frame after recovery completion");
	Expect(completedRecovery.transition == dev::EnemyAttackTransition::RecoveryCompleted, "enemy attack phase runner should report recovery completion");
	Expect(Near(enemy.stateTimerSeconds, 0.0F), "enemy attack phase runner should reset timer after recovery completion");
}

void TestEnemyAttackPhaseRunnerStartsWindup()
{
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.moveState = dev::EnemyMoveState::Recovering;
	enemy.stateTimerSeconds = 0.40F;

	dev::EnemyAttackPhaseRunner {}.startWindup(enemy);

	Expect(enemy.moveState == dev::EnemyMoveState::Attacking, "enemy attack phase runner should enter attacking state");
	Expect(Near(enemy.stateTimerSeconds, 0.0F), "enemy attack phase runner should reset timer when starting windup");
}

void TestEnemyAttackRestartPolicyUsesRecoveredRange()
{
	dev::Enemy enemy = MakeEnemy({ 0, 0 });
	enemy.tuning.attackRangeTiles = 1;
	dev::Player closeTarget = MakePlayer({ 1, 0 });
	dev::Player farTarget = MakePlayer({ 3, 0 });
	dev::EnemyAttackRestartPolicy policy;

	Expect(policy.shouldRestartAfterRecovery(enemy, closeTarget), "enemy attack restart policy should restart when target remains in range");
	Expect(!policy.shouldRestartAfterRecovery(enemy, farTarget), "enemy attack restart policy should release when target is out of range");
}

void TestEnemyAttackEventEmitterRecordsTransitions()
{
	dev::EventRecorder events;
	dev::Enemy enemy = MakeEnemy({ 1, 0 });
	enemy.id = 79;
	dev::EnemyAttackResult result {
		.consumedFrame = true,
		.transition = dev::EnemyAttackTransition::WindupCompleted,
	};

	dev::EnemyAttackEventEmitter { &events }.emit(enemy, result);

	Expect(events.events().size() == 1, "enemy attack event emitter should emit one transition event");
	if (events.events().empty())
		return;

	const dev::MovementEvent &event = events.events()[0];
	Expect(event.type == dev::MovementEventType::EnemyAttackTransitioned, "enemy attack event should report transition type");
	Expect(event.tile == dev::Point { 1, 0 }, "enemy attack event should report enemy tile");
	Expect(event.enemyId == 79, "enemy attack event should report enemy id");
	Expect(event.enemyAttackTransition == dev::EnemyAttackTransition::WindupCompleted, "enemy attack event should report attack transition");
}

void TestEnemyMovementEmitsAttackTransition()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::EventRecorder events;
	dev::Player player = MakePlayer({ 1, 0 });
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].id = 80;
	enemies[0].tuning.attackRangeTiles = 1;

	dev::EnemyMovement { map, collision, &events }.update(enemies, player, 0.016F);

	Expect(!events.events().empty(), "enemy movement should emit attack transition events");
	if (events.events().empty())
		return;

	const dev::MovementEvent &event = events.events()[0];
	Expect(event.type == dev::MovementEventType::EnemyAttackTransitioned, "enemy movement should emit enemy attack transition event");
	Expect(event.enemyId == 80, "enemy movement attack event should include enemy id");
	Expect(event.enemyAttackTransition == dev::EnemyAttackTransition::WindupStarted, "enemy movement attack event should report windup start");
}

void TestEnemyMovementReporterPublishesAttackAndPursuit()
{
	dev::EventRecorder events;
	dev::Enemy enemy = MakeEnemy({ 2, 0 });
	enemy.id = 81;
	dev::EnemyMovementReporter reporter { &events };

	reporter.reportAttack(enemy, {
	    .consumedFrame = true,
	    .transition = dev::EnemyAttackTransition::WindupStarted,
	});
	reporter.reportPursuit(enemy, {
	    .stepsCommitted = 1,
	    .stopReason = dev::EnemyPursuitStopReason::BudgetSpent,
	});

	Expect(events.events().size() == 2, "enemy movement reporter should publish attack and pursuit events");
	if (events.events().size() < 2)
		return;

	Expect(events.events()[0].type == dev::MovementEventType::EnemyAttackTransitioned, "enemy movement reporter should publish attack transition first");
	Expect(events.events()[0].enemyAttackTransition == dev::EnemyAttackTransition::WindupStarted, "enemy movement reporter should preserve attack transition");
	Expect(events.events()[1].type == dev::MovementEventType::EnemyPursuitStopped, "enemy movement reporter should publish pursuit stop");
	Expect(events.events()[1].enemyPursuitStopReason == dev::EnemyPursuitStopReason::BudgetSpent, "enemy movement reporter should preserve pursuit stop reason");
}

void TestCombatResolverDamageAndDefeat()
{
	dev::CombatStats attacker { .hitPoints = 20, .attackPower = 7, .defense = 1 };
	dev::Combatant target {
		.target = { .type = dev::TargetType::Enemy, .id = 99, .tile = { 1, 0 } },
		.stats = { .hitPoints = 5, .attackPower = 3, .defense = 2 },
	};

	dev::CombatResult result = dev::CombatResolver {}.resolveAttack(attacker, target);

	Expect(result.damage == 5, "combat damage should be attack minus defense");
	Expect(result.type == dev::CombatResultType::Defeated, "target should be defeated when hp reaches zero");
	Expect(target.stats.hitPoints == 0, "target hp should clamp to zero");
}

void TestActionExecutorAttackResolvesCombat()
{
	dev::CombatSystem combat;
	dev::Target target { .type = dev::TargetType::Enemy, .id = 7, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 6;
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionResult result = dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);
	const dev::Combatant *enemy = combat.registry().find(target);

	Expect(result.type == dev::ActionResultType::Executed, "attack action should execute through combat system");
	Expect(enemy != nullptr, "combat target should still be registered");
	Expect(enemy != nullptr && enemy->stats.hitPoints == 5, "attack action should damage combat target");
	Expect(player.destinationAction.type == dev::DestinationActionType::None, "executed combat action should clear destination action");
}

void TestCombatSystemEmitsHitEvent()
{
	dev::CombatEventRecorder combatEvents;
	dev::CombatSystem combat { &combatEvents };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 8, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 6;
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);

	const std::vector<dev::CombatEvent> &events = combatEvents.events();
	Expect(events.size() == 1, "combat hit should emit one combat event");
	Expect(events.size() == 1 && events[0].type == dev::CombatEventType::Hit, "combat event should be Hit");
	Expect(events.size() == 1 && events[0].damage == 5, "combat hit event should include damage");
	Expect(events.size() == 1 && events[0].remainingHitPoints == 5, "combat hit event should include remaining hp");
	Expect(events.size() == 1 && events[0].target.id == 8, "combat hit event should include target id");
}

void TestCombatSystemEmitsDefeatedEvent()
{
	dev::CombatEventRecorder combatEvents;
	dev::CombatSystem combat { &combatEvents };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 9, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 5, .attackPower = 3, .defense = 1 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 6;
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);

	const std::vector<dev::CombatEvent> &events = combatEvents.events();
	Expect(events.size() == 1, "defeat should emit one combat event");
	Expect(events.size() == 1 && events[0].type == dev::CombatEventType::Defeated, "combat event should be Defeated");
	Expect(events.size() == 1 && events[0].damage == 5, "defeated event should include damage");
	Expect(events.size() == 1 && events[0].remainingHitPoints == 0, "defeated event should include zero remaining hp");
}

void TestPlayerAttackUsesEquippedCombatModifiers()
{
	dev::CombatSystem combat;
	dev::Target target { .type = dev::TargetType::Enemy, .id = 10, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 10, .attackPower = 3, .defense = 2 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 5;
	player.inventory.items.push_back({
	    .id = 910,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	    .combatModifiers = { .attackPower = 99 },
	});
	player.inventory.equipment.weapon = dev::Item {
		.id = 911,
		.equipmentSlot = dev::EquipmentSlot::Weapon,
		.combatModifiers = { .attackPower = 4 },
	};
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionResult result = dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);
	const dev::Combatant *enemy = combat.registry().find(target);

	Expect(result.type == dev::ActionResultType::Executed, "equipped attack modifier should still allow action execution");
	Expect(enemy != nullptr && enemy->stats.hitPoints == 3, "equipped weapon modifier should increase player attack damage");
}

void TestEnemyAttackUsesEquippedDefenseModifiers()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::CombatSystem combat;
	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.hitPoints = 20;
	player.combatStats.defense = 1;
	player.inventory.items.push_back({
	    .id = 912,
	    .equipmentSlot = dev::EquipmentSlot::Armor,
	    .combatModifiers = { .defense = 99 },
	});
	player.inventory.equipment.armor = dev::Item {
		.id = 913,
		.equipmentSlot = dev::EquipmentSlot::Armor,
		.combatModifiers = { .defense = 4 },
	};
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].combatStats.attackPower = 8;
	enemies[0].tuning.attackRangeTiles = 1;
	enemies[0].tuning.attackWindupSeconds = 0.25F;

	dev::EnemyMovement movement { map, collision, nullptr, &combat };
	movement.update(enemies, player, 0.016F);
	movement.update(enemies, player, 0.25F);

	Expect(player.combatStats.hitPoints == 17, "equipped armor modifier should reduce enemy attack damage");
}

void TestEnemyAttackResolvesCombatAgainstPlayer()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::CombatEventRecorder combatEvents;
	dev::CombatSystem combat { &combatEvents };
	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.hitPoints = 20;
	player.combatStats.defense = 1;
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].combatStats.attackPower = 5;
	enemies[0].tuning.attackRangeTiles = 1;
	enemies[0].tuning.attackWindupSeconds = 0.25F;

	dev::EnemyMovement movement { map, collision, nullptr, &combat };
	movement.update(enemies, player, 0.016F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Attacking, "enemy should enter windup before damaging player");

	movement.update(enemies, player, 0.25F);
	Expect(player.combatStats.hitPoints == 16, "enemy attack should damage player after windup");
	Expect(enemies[0].moveState == dev::EnemyMoveState::Recovering, "enemy should recover after resolving attack");

	const std::vector<dev::CombatEvent> &events = combatEvents.events();
	Expect(events.size() == 1, "enemy attack should emit one combat event");
	Expect(events.size() == 1 && events[0].type == dev::CombatEventType::Hit, "enemy attack event should be Hit");
	Expect(events.size() == 1 && events[0].target.type == dev::TargetType::Player, "enemy attack event should target player");
	Expect(events.size() == 1 && events[0].damage == 4, "enemy attack event should include damage");
}

void TestSimulationPlayerUpdaterAdvancesPlayerMovement()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	world.movementEvents = &movementEvents;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].path.pushStep({ 1, 0 });
	world.players[0].moveState = dev::PlayerMoveState::Pathing;

	dev::SimulationPlayerUpdater {}.update(world, 0.016F);

	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation player updater should commit player path steps");
	Expect(world.players[0].moveState == dev::PlayerMoveState::Idle, "simulation player updater should settle player after final path step");
	Expect(!movementEvents.events().empty() && movementEvents.events()[0].type == dev::MovementEventType::StepCommitted, "simulation player updater should emit movement events");
}

void TestSimulationPlayerMovementRunnerWiresWorldServices()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	world.movementEvents = &movementEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].path.pushStep({ 1, 0 });
	world.players[0].moveState = dev::PlayerMoveState::Pathing;

	dev::SimulationPlayerMovementRunner {}.run(world, 0.016F);

	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation player movement runner should advance players using world movement services");
	Expect(!movementEvents.events().empty(), "simulation player movement runner should forward player movement events");
	Expect(!movementEvents.events().empty() && movementEvents.events()[0].playerId == 0, "simulation player movement runner should use world movement event sink");

	movementEvents.clear();
	combatEvents.clear();
	dev::Target target { .type = dev::TargetType::Enemy, .id = 83, .tile = { 2, 0 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});
	world.players[0].combatStats.attackPower = 6;
	world.players[0].destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	world.players[0].moveState = dev::PlayerMoveState::Acting;

	dev::SimulationPlayerMovementRunner {}.run(world, 0.016F);

	Expect(!movementEvents.events().empty() && movementEvents.events().back().type == dev::MovementEventType::ActionExecuted, "simulation player movement runner should forward action events");
	Expect(combatEvents.events().size() == 1, "simulation player movement runner should wire player actions to world combat");
	Expect(combatEvents.events().size() == 1 && combatEvents.events()[0].target.id == 83, "simulation player movement runner should resolve combat against world targets");
}

void TestSimulationEnemyTargetSelectorChoosesCurrentPlayer()
{
	dev::SimulationWorld world;
	dev::SimulationEnemyTargetSelector selector;

	Expect(selector.selectTarget(world) == nullptr, "simulation enemy target selector should return no target without players");

	world.players.push_back(MakePlayer({ 4, 0 }));
	world.players.push_back(MakePlayer({ 8, 0 }));

	dev::Player *target = selector.selectTarget(world);
	Expect(target == &world.players.front(), "simulation enemy target selector should choose the current player target");
	Expect(target != nullptr && target->position.tile == dev::Point { 4, 0 }, "simulation enemy target selector should expose the chosen player's position");
}

void TestSimulationEnemyMovementRunnerWiresWorldServices()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	world.movementEvents = &movementEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 4, 0 }));
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].id = 82;
	world.enemies[0].tuning.maxStepsPerTick = 1;
	world.enemies[0].tuning.attackRangeTiles = 0;

	dev::SimulationEnemyMovementRunner {}.run(world, world.players[0], 0.016F);

	Expect(world.enemies[0].position.tile == dev::Point { 1, 0 }, "simulation enemy movement runner should advance enemies using world movement services");
	Expect(!movementEvents.events().empty(), "simulation enemy movement runner should forward enemy movement events");
	Expect(!movementEvents.events().empty() && movementEvents.events()[0].enemyId == 82, "simulation enemy movement runner should use world movement event sink");
}

void TestSimulationEnemyUpdaterAdvancesEnemyMovement()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 4, 0 }));
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].tuning.maxStepsPerTick = 1;

	dev::SimulationEnemyUpdater {}.update(world, 0.016F);

	Expect(world.enemies[0].position.tile == dev::Point { 1, 0 }, "simulation enemy updater should advance enemies toward the player target");
	Expect(world.enemies[0].moveState == dev::EnemyMoveState::Pursuing, "simulation enemy updater should preserve enemy movement state");
}

void TestSimulationEnemyUpdaterSkipsWithoutTarget()
{
	dev::SimulationWorld world;
	world.enemies.push_back(MakeEnemy({ 0, 0 }));

	dev::SimulationEnemyUpdater {}.update(world, 0.016F);

	Expect(world.enemies[0].position.tile == dev::Point { 0, 0 }, "simulation enemy updater should not move enemies without a target player");
	Expect(world.enemies[0].moveState == dev::EnemyMoveState::Idle, "simulation enemy updater should leave enemies idle without a target player");
}

void TestSimulationActorUpdaterRunsPlayersBeforeEnemies()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 3, 0 }));
	world.players[0].path.pushStep({ 4, 0 });
	world.players[0].moveState = dev::PlayerMoveState::Pathing;
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].tuning.maxStepsPerTick = 4;
	world.enemies[0].tuning.attackRangeTiles = 0;

	dev::SimulationActorUpdater {}.update(
	    world,
	    dev::SimulationTimeStep::fromRawDelta(0.016F),
	    dev::SimulationFramePolicy::forMode(dev::SimulationMode::Gameplay));

	Expect(world.players[0].position.tile == dev::Point { 4, 0 }, "simulation actor updater should update players first");
	Expect(world.enemies[0].position.tile == dev::Point { 4, 0 }, "simulation actor updater should let enemies pursue the freshly committed player position");
}

void TestSimulationActorUpdaterCanSkipEnemies()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 3, 0 }));
	world.players[0].path.pushStep({ 4, 0 });
	world.players[0].moveState = dev::PlayerMoveState::Pathing;
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].tuning.maxStepsPerTick = 4;
	world.enemies[0].tuning.attackRangeTiles = 0;

	dev::SimulationActorUpdater {}.update(
	    world,
	    dev::SimulationTimeStep::fromRawDelta(0.016F),
	    dev::SimulationFramePolicy::forMode(dev::SimulationMode::NetworkPrediction));

	Expect(world.players[0].position.tile == dev::Point { 4, 0 }, "simulation actor updater should still update players during prediction");
	Expect(world.enemies[0].position.tile == dev::Point { 0, 0 }, "simulation actor updater should skip enemies when policy disables enemies");
}

void TestSimulationTickPipelineDrainsCommandsBeforeActors()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationTickPipeline {}.run(
	    world,
	    dev::SimulationTimeStep::fromRawDelta(0.016F),
	    dev::SimulationFramePolicy::forMode(dev::SimulationMode::Gameplay));

	dev::MovementCommand command;
	Expect(!world.commandQueue.tryPop(command), "simulation tick pipeline should drain accepted commands before actor updates");
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation tick pipeline should let drained commands affect actors in the same tick");
}

void TestSimulationTickPipelineCanSkipCommandIntake()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].path.pushStep({ 1, 0 });
	world.players[0].moveState = dev::PlayerMoveState::Pathing;
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 2, 0 },
	    .destinationAction = std::nullopt,
	});
	dev::SimulationFramePolicy policy {
	    .acceptCommands = false,
	    .updatePlayers = true,
	    .updateEnemies = false,
	};

	dev::SimulationTickPipeline {}.run(
	    world,
	    dev::SimulationTimeStep::fromRawDelta(0.016F),
	    policy);

	dev::MovementCommand command;
	Expect(world.commandQueue.tryPop(command), "simulation tick pipeline should preserve commands when intake is disabled");
	Expect(command.destination == dev::Point { 2, 0 }, "simulation tick pipeline should leave preserved command payload intact");
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation tick pipeline should still run enabled actor stages");
}

void TestSimulationTickDispatchesMovementAndCombat()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	world.movementEvents = &movementEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].combatStats.attackPower = 6;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 20, .tile = { 1, 0 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});
	world.commandQueue.push({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = target.tile,
	    .destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	});

	dev::SimulationTick {}.update(world, 0.016F);

	const dev::Combatant *enemy = world.combat.registry().find(target);
	const std::vector<dev::CombatEvent> &combat = combatEvents.events();

	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation tick should move player from queued command");
	Expect(enemy != nullptr && enemy->stats.hitPoints == 5, "simulation tick should resolve queued attack through combat");
	Expect(combat.size() == 1 && combat[0].type == dev::CombatEventType::Hit, "simulation tick should emit combat event");
	Expect(!movementEvents.events().empty(), "simulation tick should emit movement events");
}

void TestSimulationPolicyPausedDoesNotDrainCommands()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationTick tick;
	tick.update(world, 0.016F, dev::SimulationFramePolicy::forMode(dev::SimulationMode::Paused));
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "paused simulation should not move player");

	tick.update(world, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "gameplay simulation should drain preserved command");
}

void TestSimulationFramePolicyDescriberReportsModePolicy()
{
	dev::SimulationFramePolicyDescriber describer;

	const dev::SimulationFramePolicyDescription gameplay = describer.describe(dev::SimulationMode::Gameplay);
	Expect(std::string_view { gameplay.modeName } == "Gameplay", "policy describer should name gameplay mode");
	Expect(gameplay.policy.acceptCommands && gameplay.policy.updatePlayers && gameplay.policy.updateEnemies, "policy describer should report gameplay policy");
	Expect(std::string_view { gameplay.summary }.find("all actors") != std::string_view::npos, "policy describer should explain gameplay actor behavior");

	const dev::SimulationFramePolicyDescription inventory = describer.describe(dev::SimulationMode::Inventory);
	Expect(std::string_view { inventory.modeName } == "Inventory", "policy describer should name inventory mode");
	Expect(!inventory.policy.acceptCommands && !inventory.policy.updatePlayers && !inventory.policy.updateEnemies, "policy describer should report inventory policy");
	Expect(std::string_view { inventory.summary }.find("inventory owns input") != std::string_view::npos, "policy describer should explain inventory input ownership");

	const dev::SimulationFramePolicyDescription paused = describer.describe(dev::SimulationMode::Paused);
	Expect(std::string_view { paused.modeName } == "Paused", "policy describer should name paused mode");
	Expect(!paused.policy.acceptCommands && !paused.policy.updatePlayers && !paused.policy.updateEnemies, "policy describer should report paused policy");
	Expect(std::string_view { paused.summary }.find("freeze actors") != std::string_view::npos, "policy describer should explain paused actor freeze");

	const dev::SimulationFramePolicyDescription replay = describer.describe(dev::SimulationMode::Replay);
	Expect(std::string_view { replay.modeName } == "Replay", "policy describer should name replay mode");
	Expect(replay.policy.acceptCommands && replay.policy.updatePlayers && replay.policy.updateEnemies, "policy describer should report replay policy");
	Expect(std::string_view { replay.summary }.find("recorded commands") != std::string_view::npos, "policy describer should explain replay command source");

	const dev::SimulationFramePolicyDescription prediction = describer.describe(dev::SimulationMode::NetworkPrediction);
	Expect(std::string_view { prediction.modeName } == "NetworkPrediction", "policy describer should name prediction mode");
	Expect(prediction.policy.acceptCommands && prediction.policy.updatePlayers && !prediction.policy.updateEnemies, "policy describer should report prediction policy");
	Expect(std::string_view { prediction.summary }.find("without enemies") != std::string_view::npos, "policy describer should explain prediction enemy gating");
}

void TestSimulationClockHitStopFreezesActorUpdates()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationClock clock;
	clock.triggerHitStop(0.25F);
	dev::SimulationTick tick;

	tick.update(world, clock.step(0.10F));
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "hit-stop should freeze player movement");

	tick.update(world, clock.step(0.20F));
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "movement should resume after hit-stop remainder expires");
}

void TestSimulationClockScalesEnemyWindup()
{
	dev::SimulationWorld world;
	dev::CombatEventRecorder combatEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 1, 0 }));
	world.players[0].combatStats.hitPoints = 20;
	world.players[0].combatStats.defense = 1;
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].moveState = dev::EnemyMoveState::Attacking;
	world.enemies[0].combatStats.attackPower = 5;
	world.enemies[0].tuning.attackWindupSeconds = 1.0F;

	dev::SimulationClock clock;
	clock.setTimeScale(0.5F);
	dev::SimulationTick tick;

	tick.update(world, clock.step(1.0F));
	Expect(world.players[0].combatStats.hitPoints == 20, "half-speed enemy windup should not finish after one raw second");

	tick.update(world, clock.step(1.0F));
	Expect(world.players[0].combatStats.hitPoints == 16, "half-speed enemy windup should finish after two raw seconds");
	Expect(combatEvents.events().size() == 1, "scaled windup should emit one combat event when it completes");
}

void TestEffectRouterMapsMovementEventsToRequests()
{
	dev::EffectRecorder effects;
	dev::EffectRouter router { effects };

	router.route(dev::MovementEvent {
	    .type = dev::MovementEventType::StepCommitted,
	    .tile = { 2, 3 },
	});
	router.route(dev::MovementEvent {
	    .type = dev::MovementEventType::PathBlocked,
	    .tile = { 3, 3 },
	});

	const std::vector<dev::EffectRequest> &requests = effects.requests();
	Expect(requests.size() == 2, "movement effects should emit two requests");
	Expect(requests.size() == 2 && requests[0].type == dev::EffectRequestType::Footstep, "step should create footstep effect");
	Expect(requests.size() == 2 && requests[0].tile == dev::Point { 2, 3 }, "footstep should keep event tile");
	Expect(requests.size() == 2 && requests[1].type == dev::EffectRequestType::BlockedFeedback, "blocked path should create feedback effect");
}

void TestEffectRouterMapsCombatHitToRequests()
{
	dev::EffectRecorder effects;
	dev::EffectRouter router { effects };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 30, .tile = { 4, 1 } };

	router.route(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = target,
	    .damage = 7,
	    .remainingHitPoints = 3,
	    .result = dev::CombatResultType::Hit,
	});

	const std::vector<dev::EffectRequest> &requests = effects.requests();
	Expect(requests.size() == 3, "combat hit should emit damage, impact, and hit-stop requests");
	Expect(requests.size() == 3 && requests[0].type == dev::EffectRequestType::DamageNumber, "combat hit should create damage number");
	Expect(requests.size() == 3 && requests[0].damage == 7, "damage number should preserve damage");
	Expect(requests.size() == 3 && requests[1].type == dev::EffectRequestType::HitImpact, "combat hit should create impact request");
	Expect(requests.size() == 3 && requests[2].type == dev::EffectRequestType::HitStop, "combat hit should create hit-stop request");
	Expect(requests.size() == 3 && requests[2].durationSeconds > 0.0F, "hit-stop request should include duration");
}

void TestEffectApplierAppliesHitStopToClock()
{
	dev::EffectRecorder effects;
	dev::EffectRouter router { effects };
	dev::SimulationClock clock;
	dev::EffectApplier applier { &clock };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 31, .tile = { 2, 1 } };

	router.route(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = target,
	    .damage = 4,
	    .remainingHitPoints = 6,
	    .result = dev::CombatResultType::Hit,
	});

	for (const dev::EffectRequest &request : effects.requests()) {
		applier.apply(request);
	}

	Expect(clock.hitStopRemainingSeconds() > 0.0F, "hit-stop effect request should apply to simulation clock");
	dev::SimulationTimeStep step = clock.step(0.01F);
	Expect(step.playerDeltaSeconds == 0.0F, "applied hit-stop should freeze player actor time");
	Expect(step.enemyDeltaSeconds == 0.0F, "applied hit-stop should freeze enemy actor time");
}

void TestSimulationTimeStepBuilderUsesClock()
{
	dev::SimulationClock clock;
	clock.setTimeScale(0.5F);
	clock.triggerHitStop(0.25F);
	dev::SimulationTimeStepBuilder builder { &clock };

	dev::SimulationTimeStep stopped = builder.build(0.10F);
	dev::SimulationTimeStep scaled = builder.build(0.20F);

	Expect(Near(stopped.rawDeltaSeconds, 0.10F), "time step builder should preserve raw delta during hit-stop");
	Expect(stopped.playerDeltaSeconds == 0.0F, "time step builder should freeze player time through clock hit-stop");
	Expect(Near(scaled.playerDeltaSeconds, 0.025F), "time step builder should apply clock time scale after hit-stop remainder");
	Expect(Near(scaled.enemyDeltaSeconds, 0.025F), "time step builder should apply scaled actor time to enemies");
}

void TestSimulationEffectPipelineRoutesAndAppliesEffects()
{
	dev::SimulationClock clock;
	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::StepCommitted,
	    .playerId = 0,
	    .tile = { 2, 0 },
	});
	frameEvents.emit(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 45, .tile = { 2, 0 } },
	    .damage = 5,
	    .remainingHitPoints = 4,
	    .result = dev::CombatResultType::Hit,
	});

	dev::SimulationEffectPipeline { &clock }.run(frameEvents);

	Expect(frameEvents.effectRequests().size() == 4, "simulation effect pipeline should route movement and combat effects");
	Expect(frameEvents.effectRequests().size() == 4 && frameEvents.effectRequests()[0].type == dev::EffectRequestType::Footstep, "simulation effect pipeline should preserve movement effect order");
	Expect(frameEvents.effectRequests().size() == 4 && frameEvents.effectRequests()[3].type == dev::EffectRequestType::HitStop, "simulation effect pipeline should route hit-stop request");
	Expect(clock.hitStopRemainingSeconds() > 0.0F, "simulation effect pipeline should apply hit-stop to clock");
}

void TestSimulationEffectFinalizerRunsEffectConsequences()
{
	dev::SimulationClock clock;
	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::StepCommitted,
	    .playerId = 0,
	    .tile = { 3, 0 },
	});
	frameEvents.emit(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 46, .tile = { 3, 0 } },
	    .damage = 6,
	    .remainingHitPoints = 2,
	    .result = dev::CombatResultType::Hit,
	});

	dev::SimulationEffectFinalizer { &clock }.finalize(frameEvents);

	Expect(frameEvents.effectRequests().size() == 4, "simulation effect finalizer should route frame effects");
	Expect(frameEvents.effectRequests().size() == 4 && frameEvents.effectRequests()[0].type == dev::EffectRequestType::Footstep, "simulation effect finalizer should preserve routed effect order");
	Expect(clock.hitStopRemainingSeconds() > 0.0F, "simulation effect finalizer should apply clock effects");
}

void TestSimulationFrameEventCaptureCollectsForwardsAndRestoresSinks()
{
	dev::SimulationWorld world;
	dev::EventRecorder forwardedMovement;
	dev::CombatEventRecorder forwardedCombat;
	world.movementEvents = &forwardedMovement;
	world.setCombatEventSink(&forwardedCombat);

	{
		dev::SimulationFrameEventCapture capture { world };
		Expect(world.movementEvents == &capture.events(), "simulation frame event capture should install movement capture sink");
		Expect(world.combatEvents == &capture.events(), "simulation frame event capture should install combat capture sink");

		world.movementEvents->emit({
		    .type = dev::MovementEventType::StepCommitted,
		    .playerId = 0,
		    .tile = { 2, 0 },
		});
		world.combatEvents->emit({
		    .type = dev::CombatEventType::Hit,
		    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 44, .tile = { 2, 0 } },
		    .damage = 2,
		    .remainingHitPoints = 3,
		    .result = dev::CombatResultType::Hit,
		});

		Expect(capture.events().movementEvents().size() == 1, "simulation frame event capture should collect movement events");
		Expect(capture.events().combatEvents().size() == 1, "simulation frame event capture should collect combat events");
		Expect(forwardedMovement.events().size() == 1, "simulation frame event capture should forward movement events");
		Expect(forwardedCombat.events().size() == 1, "simulation frame event capture should forward combat events");
	}

	Expect(world.movementEvents == &forwardedMovement, "simulation frame event capture should restore movement sink");
	Expect(world.combatEvents == &forwardedCombat, "simulation frame event capture should restore combat sink");
}

void TestSimulationFrameFinalizerAppliesConsequences()
{
	dev::SimulationWorld world;
	dev::SimulationClock clock;
	world.players.push_back(MakePlayer({ 1, 0 }));
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 620,
	    .tile = { 1, 0 },
	});
	dev::Enemy enemy = MakeEnemy({ 2, 0 });
	enemy.id = 41;
	world.enemies.push_back(enemy);

	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::ActionExecuted,
	    .playerId = 0,
	    .tile = { 1, 0 },
	    .actionType = dev::DestinationActionType::Pickup,
	    .actionResult = dev::ActionResultType::Executed,
	    .target = dev::Target { .type = dev::TargetType::Item, .id = 620, .tile = { 1, 0 } },
	});
	frameEvents.emit(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 41, .tile = { 2, 0 } },
	    .damage = 3,
	    .remainingHitPoints = 4,
	    .result = dev::CombatResultType::Hit,
	});

	dev::SimulationFrameFinalizer { &clock }.finalize(world, frameEvents);

	Expect(world.items.empty(), "simulation frame finalizer should apply pickup transfers");
	Expect(world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 620, "simulation frame finalizer should preserve picked item identity");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "simulation frame finalizer should remove picked item target");
	Expect(world.targets.resolveAtTile({ 2, 0 }).type == dev::TargetType::Enemy, "simulation frame finalizer should publish current enemy targets");
	Expect(!frameEvents.effectRequests().empty(), "simulation frame finalizer should route events into effect requests");
	Expect(clock.hitStopRemainingSeconds() > 0.0F, "simulation frame finalizer should apply hit-stop effect requests");
}

void TestSimulationFrameTickRunnerCollectsTickEventsAndRestoresSinks()
{
	dev::SimulationWorld world;
	dev::EventRecorder forwardedMovement;
	dev::CombatEventRecorder forwardedCombat;
	world.movementEvents = &forwardedMovement;
	world.setCombatEventSink(&forwardedCombat);
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::SimulationFrameEvents frameEvents = dev::SimulationFrameTickRunner {}.run(
	    world,
	    dev::SimulationTimeStep::fromRawDelta(1.0F),
	    dev::SimulationFramePolicy::forMode(dev::SimulationMode::Gameplay));

	Expect(!frameEvents.movementEvents().empty(), "simulation frame tick runner should collect tick movement events");
	Expect(!forwardedMovement.events().empty(), "simulation frame tick runner should forward collected movement events");
	Expect(world.movementEvents == &forwardedMovement, "simulation frame tick runner should restore movement sink after tick");
	Expect(world.combatEvents == &forwardedCombat, "simulation frame tick runner should restore combat sink after tick");
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation frame tick runner should still run the simulation tick");
}

void TestSimulationTargetFinalizerSynchronizesTargets()
{
	dev::SimulationWorld world;
	world.enemies.push_back(MakeEnemy({ 2, 0 }));
	world.enemies[0].id = 51;
	world.targets.add({ .type = dev::TargetType::Enemy, .id = 50, .tile = { 0, 0 } });
	world.targets.add({ .type = dev::TargetType::Object, .id = 60, .tile = { 4, 0 } });
	world.combat.registry().add({
	    .target = { .type = dev::TargetType::Enemy, .id = 51, .tile = { 2, 0 } },
	    .stats = { .hitPoints = 5, .attackPower = 1, .defense = 0 },
	});
	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit(dev::CombatEvent {
	    .type = dev::CombatEventType::Defeated,
	    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 51, .tile = { 2, 0 } },
	    .damage = 5,
	    .remainingHitPoints = 0,
	    .result = dev::CombatResultType::Defeated,
	});

	dev::SimulationTargetFinalizer {}.finalize(world, frameEvents);

	Expect(world.targets.resolveAtTile({ 0, 0 }).type == dev::TargetType::EmptyTile, "simulation target finalizer should remove stale enemy targets");
	Expect(world.targets.resolveAtTile({ 2, 0 }).type == dev::TargetType::EmptyTile, "simulation target finalizer should remove defeated enemy target");
	Expect(world.targets.resolveAtTile({ 4, 0 }).type == dev::TargetType::Object, "simulation target finalizer should preserve unrelated targets");
}

void TestSimulationInventoryFinalizerAppliesPickupConsequences()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 1, 0 }));
	world.players.push_back(MakePlayer({ 3, 0 }));
	world.players[1].inventory.capacity = 1;
	world.players[1].inventory.items.push_back({ .id = 800, .tile = { 3, 0 } });
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 630,
	    .tile = { 1, 0 },
	});
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 631,
	    .tile = { 3, 0 },
	});

	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::ActionExecuted,
	    .playerId = 0,
	    .tile = { 1, 0 },
	    .actionType = dev::DestinationActionType::Pickup,
	    .actionResult = dev::ActionResultType::Executed,
	    .target = dev::Target { .type = dev::TargetType::Item, .id = 630, .tile = { 1, 0 } },
	});
	frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::ActionExecuted,
	    .playerId = 1,
	    .tile = { 3, 0 },
	    .actionType = dev::DestinationActionType::Pickup,
	    .actionResult = dev::ActionResultType::Executed,
	    .target = dev::Target { .type = dev::TargetType::Item, .id = 631, .tile = { 3, 0 } },
	});

	std::vector<dev::InventoryTransferResult> results = dev::SimulationInventoryFinalizer {}.finalize(world, frameEvents);

	Expect(results.size() == 2, "simulation inventory finalizer should report every pickup consequence");
	Expect(results.size() == 2 && results[0].type == dev::InventoryTransferResultType::Transferred, "simulation inventory finalizer should report transferred pickups");
	Expect(results.size() == 2 && results[1].type == dev::InventoryTransferResultType::RejectedFull, "simulation inventory finalizer should report rejected pickups");
	Expect(world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 630, "simulation inventory finalizer should transfer accepted pickup to player");
	Expect(world.items.size() == 1 && world.items[0].id == 631, "simulation inventory finalizer should keep rejected item in world");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "simulation inventory finalizer should remove accepted item target");
	Expect(world.targets.resolveAtTile({ 3, 0 }).type == dev::TargetType::Item, "simulation inventory finalizer should preserve rejected item target");
}

void TestSimulationFrameRunnerProcessesConsequences()
{
	dev::SimulationWorld world;
	dev::EventRecorder forwardedMovement;
	dev::CombatEventRecorder forwardedCombat;
	dev::SimulationClock clock;
	world.movementEvents = &forwardedMovement;
	world.setCombatEventSink(&forwardedCombat);
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].combatStats.attackPower = 6;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 40, .tile = { 1, 0 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});
	world.commandQueue.push({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = target.tile,
	    .destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	});

	dev::SimulationFrameRunner runner { &clock };
	dev::SimulationFrameEvents frame = runner.run(world, 0.016F);
	const dev::Combatant *enemy = world.combat.registry().find(target);

	Expect(enemy != nullptr && enemy->stats.hitPoints == 5, "frame runner should preserve combat registry while collecting events");
	Expect(!frame.movementEvents().empty(), "frame runner should collect movement events");
	Expect(frame.combatEvents().size() == 1, "frame runner should collect combat events");
	Expect(!frame.effectRequests().empty(), "frame runner should route frame events into effect requests");
	Expect(clock.hitStopRemainingSeconds() > 0.0F, "frame runner should apply hit-stop effect requests to clock");
	Expect(!forwardedMovement.events().empty(), "frame runner should forward movement events to existing sink");
	Expect(forwardedCombat.events().size() == 1, "frame runner should forward combat events to existing sink");

	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 2, 0 },
	    .destinationAction = std::nullopt,
	});
	dev::SimulationFrameEvents stoppedFrame = runner.run(world, 0.01F);
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "runner should freeze actor updates while hit-stop remains");
	Expect(!stoppedFrame.movementEvents().empty(), "runner should still accept commands during hit-stop");
}

void TestTargetRegistryResolvesAndRemovesTargets()
{
	dev::TargetRegistry registry;
	registry.add({ .type = dev::TargetType::Item, .id = 10, .tile = { 2, 3 } });
	registry.add({ .type = dev::TargetType::Enemy, .id = 11, .tile = { 4, 5 } });

	dev::Target item = registry.resolveAtTile({ 2, 3 });
	dev::Target enemy = registry.resolveAtTile({ 4, 5 });
	dev::Target empty = registry.resolveAtTile({ 8, 8 });

	Expect(item.type == dev::TargetType::Item && item.id == 10, "target registry should resolve item target by tile");
	Expect(enemy.type == dev::TargetType::Enemy && enemy.id == 11, "target registry should resolve enemy target by tile");
	Expect(empty.type == dev::TargetType::EmptyTile && empty.tile == dev::Point { 8, 8 }, "target registry should return empty tile when no target is registered");
	Expect(registry.remove(dev::TargetType::Item, 10), "target registry should remove matching target");
	Expect(!registry.remove(dev::TargetType::Item, 10), "target registry should report missing target removal");
	Expect(registry.resolveAtTile({ 2, 3 }).type == dev::TargetType::EmptyTile, "removed target should no longer resolve");
	Expect(registry.removeAll(dev::TargetType::Enemy) == 1, "target registry should remove all targets of a type");
	Expect(registry.empty(), "target registry should be empty after removing remaining type");
}

void TestTargetSynchronizerSyncsEnemyTargetsWithoutRemovingObjects()
{
	dev::TargetRegistry registry;
	registry.add({ .type = dev::TargetType::Object, .id = 90, .tile = { 8, 8 } });
	registry.add({ .type = dev::TargetType::Enemy, .id = 1, .tile = { 0, 0 } });

	std::vector<dev::Enemy> enemies { MakeEnemy({ 3, 4 }) };
	enemies[0].id = 12;
	dev::CombatRegistry combat;
	combat.add({
	    .target = { .type = dev::TargetType::Enemy, .id = 12, .tile = { 3, 4 } },
	    .stats = { .hitPoints = 5, .attackPower = 2, .defense = 1 },
	});

	dev::TargetSynchronizer synchronizer;
	synchronizer.syncEnemyTargets(enemies, combat, registry);

	dev::Target enemy = registry.resolveAtTile({ 3, 4 });
	dev::Target stale = registry.resolveAtTile({ 0, 0 });
	dev::Target object = registry.resolveAtTile({ 8, 8 });
	Expect(enemy.type == dev::TargetType::Enemy && enemy.id == 12, "target synchronizer should publish current enemy target");
	Expect(stale.type == dev::TargetType::EmptyTile, "target synchronizer should remove stale enemy target");
	Expect(object.type == dev::TargetType::Object && object.id == 90, "target synchronizer should preserve non-enemy targets");
}

void TestTargetSynchronizerSkipsDefeatedEnemies()
{
	dev::TargetRegistry registry;
	std::vector<dev::Enemy> enemies { MakeEnemy({ 4, 4 }), MakeEnemy({ 5, 4 }) };
	enemies[0].id = 20;
	enemies[1].id = 21;

	dev::CombatRegistry combat;
	combat.add({
	    .target = { .type = dev::TargetType::Enemy, .id = 20, .tile = { 4, 4 } },
	    .stats = { .hitPoints = 0, .attackPower = 2, .defense = 1 },
	});
	combat.add({
	    .target = { .type = dev::TargetType::Enemy, .id = 21, .tile = { 5, 4 } },
	    .stats = { .hitPoints = 3, .attackPower = 2, .defense = 1 },
	});

	dev::TargetSynchronizer {}.syncEnemyTargets(enemies, combat, registry);

	Expect(registry.resolveAtTile({ 4, 4 }).type == dev::TargetType::EmptyTile, "target synchronizer should skip defeated combat registry enemies");
	Expect(registry.resolveAtTile({ 5, 4 }).type == dev::TargetType::Enemy, "target synchronizer should keep living combat registry enemies");
}

void TestFrameRunnerSynchronizesMovedEnemyTargets()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 3, 0 }));
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].id = 30;
	world.enemies[0].tuning.maxStepsPerTick = 1;
	world.enemies[0].tuning.attackRangeTiles = 0;
	world.targets.add({ .type = dev::TargetType::Enemy, .id = 30, .tile = { 0, 0 } });

	dev::SimulationFrameRunner {}.run(world, 1.0F);

	Expect(world.enemies[0].position.tile == dev::Point { 1, 0 }, "frame runner sync test should move enemy one step");
	Expect(world.targets.resolveAtTile({ 0, 0 }).type == dev::TargetType::EmptyTile, "frame runner should remove stale enemy target tile");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::Enemy, "frame runner should sync enemy target to moved tile");
}

void TestFrameRunnerRemovesDefeatedEnemyTargets()
{
	dev::SimulationWorld world;
	dev::Player player = MakePlayer({ 0, 0 });
	player.combatStats.attackPower = 10;
	world.players.push_back(player);
	world.enemies.push_back(MakeEnemy({ 1, 0 }));
	world.enemies[0].id = 40;
	world.enemies[0].tuning.attackRangeTiles = 0;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 40, .tile = { 1, 0 } };
	world.targets.add(target);
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 1, .attackPower = 1, .defense = 0 },
	});
	world.commandQueue.push({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = target.tile,
	    .destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	});

	dev::SimulationFrameEvents frame = dev::SimulationFrameRunner {}.run(world, 1.0F);

	bool sawDefeated = false;
	for (const dev::CombatEvent &event : frame.combatEvents()) {
		if (event.type == dev::CombatEventType::Defeated && event.target.id == 40)
			sawDefeated = true;
	}
	Expect(sawDefeated, "frame runner defeat sync test should defeat enemy target");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "frame runner should remove defeated enemy target");
}

void TestWorldEntityServiceSpawnsEnemyStateAcrossRegistries()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;

	dev::Enemy &enemy = entities.spawnEnemy(world, {
	    .id = 500,
	    .tile = { 6, 2 },
	    .tuning = { .maxStepsPerTick = 2, .attackRangeTiles = 1, .attackWindupSeconds = 0.25F, .attackRecoverySeconds = 0.75F },
	    .combatStats = { .hitPoints = 13, .attackPower = 6, .defense = 2 },
	});

	dev::Target target { .type = dev::TargetType::Enemy, .id = 500, .tile = { 6, 2 } };
	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target clickable = world.targets.resolveAtTile({ 6, 2 });

	Expect(world.enemies.size() == 1, "world entity service should add enemy to world");
	Expect(enemy.id == 500 && enemy.position.tile == dev::Point { 6, 2 }, "world entity service should initialize enemy identity and position");
	Expect(enemy.position.future == dev::Point { 6, 2 } && enemy.position.previous == dev::Point { 6, 2 }, "world entity service should initialize full actor position");
	Expect(enemy.tuning.maxStepsPerTick == 2 && enemy.tuning.attackRecoverySeconds == 0.75F, "world entity service should apply enemy tuning");
	Expect(enemy.combatStats.hitPoints == 13 && enemy.combatStats.attackPower == 6, "world entity service should apply enemy combat stats");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 13, "world entity service should register enemy combatant");
	Expect(clickable.type == dev::TargetType::Enemy && clickable.id == 500, "world entity service should register enemy target");
}

void TestWorldEntityServiceDespawnsEnemyStateAcrossRegistries()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;
	entities.spawnEnemy(world, {
	    .id = 501,
	    .tile = { 2, 6 },
	    .combatStats = { .hitPoints = 4, .attackPower = 2, .defense = 1 },
	});
	world.targets.add({ .type = dev::TargetType::Object, .id = 77, .tile = { 9, 9 } });
	world.combat.registry().add({
	    .target = { .type = dev::TargetType::Object, .id = 77, .tile = { 9, 9 } },
	    .stats = { .hitPoints = 10, .attackPower = 0, .defense = 0 },
	});

	Expect(entities.despawnEnemy(world, 501), "world entity service should report despawned enemy");

	dev::Target enemyTarget { .type = dev::TargetType::Enemy, .id = 501, .tile = { 2, 6 } };
	dev::Target objectTarget { .type = dev::TargetType::Object, .id = 77, .tile = { 9, 9 } };
	Expect(world.enemies.empty(), "world entity service should remove enemy from world");
	Expect(world.combat.registry().find(enemyTarget) == nullptr, "world entity service should remove enemy combatant");
	Expect(world.targets.resolveAtTile({ 2, 6 }).type == dev::TargetType::EmptyTile, "world entity service should remove enemy target");
	Expect(world.targets.resolveAtTile({ 9, 9 }).type == dev::TargetType::Object, "world entity service should preserve unrelated targets");
	Expect(world.combat.registry().find(objectTarget) != nullptr, "world entity service should preserve unrelated combatants");
	Expect(!entities.despawnEnemy(world, 501), "world entity service should report missing enemy on second despawn");
}

void TestWorldEntityServiceRespawnReplacesStaleEnemyState()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;
	entities.spawnEnemy(world, {
	    .id = 502,
	    .tile = { 1, 1 },
	    .combatStats = { .hitPoints = 3, .attackPower = 1, .defense = 0 },
	});
	entities.spawnEnemy(world, {
	    .id = 502,
	    .tile = { 5, 1 },
	    .combatStats = { .hitPoints = 9, .attackPower = 4, .defense = 1 },
	});

	dev::Target newTarget { .type = dev::TargetType::Enemy, .id = 502, .tile = { 5, 1 } };
	const dev::Combatant *combatant = world.combat.registry().find(newTarget);

	Expect(world.enemies.size() == 1, "world entity service respawn should keep one enemy for duplicate id");
	Expect(world.enemies.size() == 1 && world.enemies[0].position.tile == dev::Point { 5, 1 }, "world entity service respawn should update enemy tile");
	Expect(world.targets.resolveAtTile({ 1, 1 }).type == dev::TargetType::EmptyTile, "world entity service respawn should remove old target tile");
	Expect(world.targets.resolveAtTile({ 5, 1 }).type == dev::TargetType::Enemy, "world entity service respawn should add new target tile");
	Expect(world.combat.registry().combatants().size() == 1, "world entity service respawn should keep one combatant for duplicate id");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 9, "world entity service respawn should replace combat stats");
}

void TestWorldEntityServiceSpawnsAndDespawnsItems()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;

	dev::Item &item = entities.spawnItem(world, {
	    .id = 600,
	    .tile = { 4, 7 },
	});

	dev::Target clickable = world.targets.resolveAtTile({ 4, 7 });
	Expect(world.items.size() == 1, "world entity service should add item to world");
	Expect(item.id == 600 && item.tile == dev::Point { 4, 7 }, "world entity service should initialize item identity and tile");
	Expect(clickable.type == dev::TargetType::Item && clickable.id == 600, "world entity service should register item target");

	Expect(entities.despawnItem(world, 600), "world entity service should report despawned item");
	Expect(world.items.empty(), "world entity service should remove item from world");
	Expect(world.targets.resolveAtTile({ 4, 7 }).type == dev::TargetType::EmptyTile, "world entity service should remove item target");
	Expect(!entities.despawnItem(world, 600), "world entity service should report missing item on second despawn");
}

void TestWorldEntityServiceRespawnReplacesStaleItemTarget()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;
	entities.spawnItem(world, {
	    .id = 601,
	    .tile = { 2, 2 },
	});
	entities.spawnItem(world, {
	    .id = 601,
	    .tile = { 8, 2 },
	});

	Expect(world.items.size() == 1, "world entity service item respawn should keep one item for duplicate id");
	Expect(world.items.size() == 1 && world.items[0].tile == dev::Point { 8, 2 }, "world entity service item respawn should update tile");
	Expect(world.targets.resolveAtTile({ 2, 2 }).type == dev::TargetType::EmptyTile, "world entity service item respawn should remove old target tile");
	Expect(world.targets.resolveAtTile({ 8, 2 }).type == dev::TargetType::Item, "world entity service item respawn should add new target tile");
}

void TestInventoryServiceTransfersExecutedPickupToPlayerInventory()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 1, 0 }));
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 620,
	    .tile = { 1, 0 },
	});

	dev::MovementEvent pickup {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 0,
		.tile = { 1, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 620, .tile = { 1, 0 } },
	};

	std::vector<dev::InventoryTransferResult> results = dev::InventoryService {}.applyPickupEvents(world, { pickup });

	Expect(results.size() == 1 && results[0].type == dev::InventoryTransferResultType::Transferred, "inventory service should report transferred pickup");
	Expect(world.items.empty(), "inventory service should remove picked item from world");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "inventory service should remove picked item target");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1, "inventory service should add picked item to player inventory");
	Expect(world.players.size() == 1 && world.players[0].inventory.items[0].id == 620, "inventory service should preserve picked item identity");
}

void TestInventoryServiceIgnoresInvalidPickupEvents()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 621,
	    .tile = { 2, 0 },
	});

	dev::MovementEvent missingItem {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 0,
		.tile = { 0, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 999, .tile = { 9, 9 } },
	};
	dev::MovementEvent wrongPlayer {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 2,
		.tile = { 0, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 621, .tile = { 2, 0 } },
	};

	std::vector<dev::InventoryTransferResult> results = dev::InventoryService {}.applyPickupEvents(world, { missingItem, wrongPlayer });

	Expect(results.size() == 2, "inventory service should report invalid pickup attempts");
	Expect(results.size() == 2 && results[0].type == dev::InventoryTransferResultType::MissingItem, "inventory service should report missing item");
	Expect(results.size() == 2 && results[1].type == dev::InventoryTransferResultType::InvalidEvent, "inventory service should report invalid player");
	Expect(world.items.size() == 1, "inventory service should keep world item when pickup event is invalid");
	Expect(world.players[0].inventory.items.empty(), "inventory service should not add invalid pickup to inventory");
	Expect(world.targets.resolveAtTile({ 2, 0 }).type == dev::TargetType::Item, "inventory service should keep target for invalid pickup");
}

void TestInventoryServiceRejectsPickupWhenInventoryIsFull()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 1, 0 }));
	world.players[0].inventory.capacity = 1;
	world.players[0].inventory.items.push_back({ .id = 700, .tile = { 0, 0 } });
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 701,
	    .tile = { 1, 0 },
	});

	dev::MovementEvent pickup {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 0,
		.tile = { 1, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 701, .tile = { 1, 0 } },
	};

	std::vector<dev::InventoryTransferResult> results = dev::InventoryService {}.applyPickupEvents(world, { pickup });

	Expect(results.size() == 1 && results[0].type == dev::InventoryTransferResultType::RejectedFull, "inventory service should reject pickup when inventory is full");
	Expect(world.items.size() == 1 && world.items[0].id == 701, "full inventory pickup should leave item in world");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::Item, "full inventory pickup should leave item target");
	Expect(world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 700, "full inventory pickup should preserve existing inventory");
}

void TestEquipmentServiceEquipsItemFromInventory()
{
	dev::Inventory inventory;
	inventory.items.push_back({ .id = 900, .equipmentSlot = dev::EquipmentSlot::Weapon });

	dev::EquipmentResult result = dev::EquipmentService {}.equip(inventory, 900);

	Expect(result.type == dev::EquipmentResultType::Equipped, "equipment service should equip equippable item");
	Expect(result.slot == dev::EquipmentSlot::Weapon, "equipment service should report equipped slot");
	Expect(inventory.items.empty(), "equipment service should remove equipped item from bag");
	Expect(inventory.equipment.weapon.has_value() && inventory.equipment.weapon->id == 900, "equipment service should place item in weapon slot");
}

void TestEquipmentServiceSwapsOccupiedSlot()
{
	dev::Inventory inventory;
	inventory.equipment.weapon = dev::Item { .id = 901, .equipmentSlot = dev::EquipmentSlot::Weapon };
	inventory.items.push_back({ .id = 902, .equipmentSlot = dev::EquipmentSlot::Weapon });

	dev::EquipmentResult result = dev::EquipmentService {}.equip(inventory, 902);

	Expect(result.type == dev::EquipmentResultType::Equipped, "equipment service should equip replacement item");
	Expect(inventory.equipment.weapon.has_value() && inventory.equipment.weapon->id == 902, "equipment service should replace occupied slot");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 901, "equipment service should return old equipment to bag");
}

void TestEquipmentServiceRejectsMissingAndNotEquippableItems()
{
	dev::Inventory inventory;
	inventory.items.push_back({ .id = 903 });

	dev::EquipmentResult missing = dev::EquipmentService {}.equip(inventory, 999);
	dev::EquipmentResult notEquippable = dev::EquipmentService {}.equip(inventory, 903);

	Expect(missing.type == dev::EquipmentResultType::MissingItem, "equipment service should reject missing item");
	Expect(notEquippable.type == dev::EquipmentResultType::NotEquippable, "equipment service should reject item without equipment slot");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 903, "equipment service should keep rejected item in bag");
	Expect(!inventory.equipment.weapon.has_value(), "equipment service should not equip rejected item");
}

void TestEquipmentServiceUnequipsWhenInventoryHasCapacity()
{
	dev::Inventory inventory;
	inventory.capacity = 2;
	inventory.equipment.armor = dev::Item { .id = 904, .equipmentSlot = dev::EquipmentSlot::Armor };

	dev::EquipmentResult result = dev::EquipmentService {}.unequip(inventory, dev::EquipmentSlot::Armor);

	Expect(result.type == dev::EquipmentResultType::Unequipped, "equipment service should unequip item when bag has capacity");
	Expect(!inventory.equipment.armor.has_value(), "equipment service should clear unequipped slot");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 904, "equipment service should return unequipped item to bag");
}

void TestEquipmentServiceRejectsUnequipWhenInventoryFull()
{
	dev::Inventory inventory;
	inventory.capacity = 1;
	inventory.items.push_back({ .id = 905 });
	inventory.equipment.accessory = dev::Item { .id = 906, .equipmentSlot = dev::EquipmentSlot::Accessory };

	dev::EquipmentResult result = dev::EquipmentService {}.unequip(inventory, dev::EquipmentSlot::Accessory);

	Expect(result.type == dev::EquipmentResultType::InventoryFull, "equipment service should reject unequip when bag is full");
	Expect(inventory.equipment.accessory.has_value() && inventory.equipment.accessory->id == 906, "equipment service should keep item equipped when unequip rejects");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 905, "equipment service should preserve full bag contents");
}

void TestEquipmentStatsServiceBuildsEffectiveCombatStats()
{
	dev::Player player = MakePlayer();
	player.combatStats.attackPower = 5;
	player.combatStats.defense = 1;
	player.inventory.items.push_back({
	    .id = 920,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	    .combatModifiers = { .attackPower = 99, .defense = 99 },
	});
	player.inventory.equipment.weapon = dev::Item {
		.id = 921,
		.equipmentSlot = dev::EquipmentSlot::Weapon,
		.combatModifiers = { .attackPower = 3 },
	};
	player.inventory.equipment.armor = dev::Item {
		.id = 922,
		.equipmentSlot = dev::EquipmentSlot::Armor,
		.combatModifiers = { .defense = 2 },
	};
	player.inventory.equipment.accessory = dev::Item {
		.id = 923,
		.equipmentSlot = dev::EquipmentSlot::Accessory,
		.combatModifiers = { .attackPower = 1, .defense = 1 },
	};

	dev::CombatStats effective = dev::EquipmentStatsService {}.effectiveCombatStats(player);

	Expect(effective.hitPoints == player.combatStats.hitPoints, "equipment stats service should preserve current hit points");
	Expect(effective.attackPower == 9, "equipment stats service should add equipped attack modifiers");
	Expect(effective.defense == 4, "equipment stats service should add equipped defense modifiers");
	Expect(player.combatStats.attackPower == 5, "equipment stats service should not mutate base combat stats");
}

void TestInventoryCommandDispatcherEquipsItem()
{
	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 930,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandResult result = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 930,
	});

	Expect(result.type == dev::InventoryCommandResultType::Applied, "inventory command dispatcher should apply valid equip command");
	Expect(result.equipmentResult.type == dev::EquipmentResultType::Equipped, "inventory command dispatcher should expose equipment result");
	Expect(player.inventory.items.empty(), "inventory command dispatcher equip should remove item from bag");
	Expect(player.inventory.equipment.weapon.has_value() && player.inventory.equipment.weapon->id == 930, "inventory command dispatcher equip should fill equipment slot");
}

void TestInventoryCommandDispatcherUnequipsSlot()
{
	dev::Player player = MakePlayer();
	player.inventory.capacity = 2;
	player.inventory.equipment.armor = dev::Item {
		.id = 931,
		.equipmentSlot = dev::EquipmentSlot::Armor,
	};

	dev::InventoryCommandResult result = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Armor,
	});

	Expect(result.type == dev::InventoryCommandResultType::Applied, "inventory command dispatcher should apply valid unequip command");
	Expect(result.equipmentResult.type == dev::EquipmentResultType::Unequipped, "inventory command dispatcher should expose unequip result");
	Expect(!player.inventory.equipment.armor.has_value(), "inventory command dispatcher unequip should clear slot");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 931, "inventory command dispatcher unequip should return item to bag");
}

void TestInventoryCommandDispatcherRejectsInvalidCommands()
{
	dev::Player player = MakePlayer();
	player.inventory.items.push_back({ .id = 932 });

	dev::InventoryCommandResult missingPayload = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	});
	dev::InventoryCommandResult notEquippable = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 932,
	});
	dev::InventoryCommandResult emptySlot = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Accessory,
	});

	Expect(missingPayload.type == dev::InventoryCommandResultType::Rejected, "inventory command dispatcher should reject missing item payload");
	Expect(notEquippable.type == dev::InventoryCommandResultType::Rejected, "inventory command dispatcher should reject failed equip service result");
	Expect(notEquippable.equipmentResult.type == dev::EquipmentResultType::NotEquippable, "inventory command dispatcher should preserve rejected equipment reason");
	Expect(emptySlot.type == dev::InventoryCommandResultType::Rejected, "inventory command dispatcher should reject failed unequip service result");
	Expect(emptySlot.equipmentResult.type == dev::EquipmentResultType::EmptySlot, "inventory command dispatcher should preserve empty slot reason");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 932, "inventory command dispatcher should not mutate inventory for rejected commands");
}

void TestInventoryCommandDispatcherEmitsInventoryEvents()
{
	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 933,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	player.inventory.items.push_back({ .id = 934 });
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };

	dev::InventoryCommandResult equipped = dispatcher.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 933,
	});
	dev::InventoryCommandResult rejected = dispatcher.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 934,
	});

	const std::vector<dev::InventoryEvent> &recorded = events.events();
	Expect(equipped.type == dev::InventoryCommandResultType::Applied, "inventory event test equip command should apply");
	Expect(rejected.type == dev::InventoryCommandResultType::Rejected, "inventory event test second equip command should reject");
	Expect(recorded.size() == 2, "inventory command dispatcher should emit one inventory event per command");
	Expect(recorded.size() == 2 && recorded[0].type == dev::InventoryEventType::Equipped, "inventory command dispatcher should emit equipped event");
	Expect(recorded.size() == 2 && recorded[0].commandType == dev::InventoryCommandType::EquipItem, "inventory equipped event should preserve command type");
	Expect(recorded.size() == 2 && recorded[0].commandResult == dev::InventoryCommandResultType::Applied, "inventory equipped event should report applied command");
	Expect(recorded.size() == 2 && recorded[0].equipmentResult == dev::EquipmentResultType::Equipped, "inventory equipped event should preserve equipment result");
	Expect(recorded.size() == 2 && recorded[0].itemId == 933, "inventory equipped event should preserve item id");
	Expect(recorded.size() == 2 && recorded[0].slot == dev::EquipmentSlot::Weapon, "inventory equipped event should preserve slot");
	Expect(recorded.size() == 2 && recorded[1].type == dev::InventoryEventType::Rejected, "inventory command dispatcher should emit rejected event");
	Expect(recorded.size() == 2 && recorded[1].equipmentResult == dev::EquipmentResultType::NotEquippable, "inventory rejected event should preserve rejection reason");
	Expect(recorded.size() == 2 && recorded[1].itemId == 934, "inventory rejected event should preserve rejected item id");
}

void TestInventoryCommandEventEmitterMapsResultsToEvents()
{
	dev::InventoryEventRecorder events;
	dev::InventoryCommandEventEmitter emitter { &events };

	emitter.emit({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = {
	        .type = dev::InventoryCommandType::EquipItem,
	        .itemId = 936,
	    },
	    .equipmentResult = {
	        .type = dev::EquipmentResultType::Equipped,
	        .itemId = 936,
	        .slot = dev::EquipmentSlot::Weapon,
	    },
	});
	emitter.emit({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = {
	        .type = dev::InventoryCommandType::UnequipSlot,
	        .slot = dev::EquipmentSlot::Armor,
	    },
	    .equipmentResult = {
	        .type = dev::EquipmentResultType::EmptySlot,
	        .slot = dev::EquipmentSlot::Armor,
	    },
	});

	const std::vector<dev::InventoryEvent> &recorded = events.events();
	Expect(recorded.size() == 2, "inventory command event emitter should publish one event per result");
	Expect(recorded.size() == 2 && recorded[0].type == dev::InventoryEventType::Equipped, "inventory command event emitter should map equipped result to equipped event");
	Expect(recorded.size() == 2 && recorded[0].commandType == dev::InventoryCommandType::EquipItem, "inventory command event emitter should preserve applied command type");
	Expect(recorded.size() == 2 && recorded[0].commandResult == dev::InventoryCommandResultType::Applied, "inventory command event emitter should preserve applied result type");
	Expect(recorded.size() == 2 && recorded[0].equipmentResult == dev::EquipmentResultType::Equipped, "inventory command event emitter should preserve equipment result");
	Expect(recorded.size() == 2 && recorded[0].itemId == 936, "inventory command event emitter should preserve item id");
	Expect(recorded.size() == 2 && recorded[0].slot == dev::EquipmentSlot::Weapon, "inventory command event emitter should preserve applied slot");
	Expect(recorded.size() == 2 && recorded[1].type == dev::InventoryEventType::Rejected, "inventory command event emitter should map rejected result to rejected event");
	Expect(recorded.size() == 2 && recorded[1].commandType == dev::InventoryCommandType::UnequipSlot, "inventory command event emitter should preserve rejected command type");
	Expect(recorded.size() == 2 && recorded[1].commandResult == dev::InventoryCommandResultType::Rejected, "inventory command event emitter should preserve rejected result type");
	Expect(recorded.size() == 2 && recorded[1].equipmentResult == dev::EquipmentResultType::EmptySlot, "inventory command event emitter should preserve rejection reason");
	Expect(recorded.size() == 2 && recorded[1].slot == dev::EquipmentSlot::Armor, "inventory command event emitter should preserve rejected slot");
}

void TestInventoryCommandCodecRoundTripsCommands()
{
	dev::InventoryCommandCodec codec;
	std::vector<dev::InventoryCommand> commands {
		{
		    .type = dev::InventoryCommandType::EquipItem,
		    .itemId = 960,
		},
		{
		    .type = dev::InventoryCommandType::UnequipSlot,
		    .slot = dev::EquipmentSlot::Accessory,
		},
	};

	for (const dev::InventoryCommand &command : commands) {
		dev::InventoryCommandPacket packet = codec.toPacket(command);
		dev::InventoryCommandBytes bytes = codec.encode(packet);
		std::optional<dev::InventoryCommandPacket> decodedPacket = codec.decode(bytes);
		Expect(decodedPacket.has_value(), "inventory command packet should decode");
		std::optional<dev::InventoryCommand> decoded = decodedPacket.has_value()
		    ? codec.fromPacket(*decodedPacket)
		    : std::nullopt;
		Expect(decoded.has_value(), "inventory command packet should become command");
		if (!decoded.has_value())
			continue;
		Expect(decoded->type == command.type, "inventory command codec should preserve command type");
		Expect(decoded->itemId == command.itemId, "inventory command codec should preserve item id");
		Expect(decoded->slot == command.slot, "inventory command codec should preserve equipment slot");
	}
}

void TestInventoryCommandCodecRejectsInvalidPackets()
{
	dev::InventoryCommandCodec codec;

	dev::InventoryCommandPacket invalidType {
		.commandType = 99,
	};
	Expect(!codec.fromPacket(invalidType).has_value(), "inventory command codec should reject invalid command type");

	dev::InventoryCommandPacket equipWithoutItem {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
	};
	Expect(!codec.fromPacket(equipWithoutItem).has_value(), "inventory command codec should reject equip without item payload");

	dev::InventoryCommandPacket unequipWithoutSlot {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
	};
	Expect(!codec.fromPacket(unequipWithoutSlot).has_value(), "inventory command codec should reject unequip without slot payload");

	dev::InventoryCommandPacket invalidSlot {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
		.hasSlot = 1,
		.slot = 99,
	};
	Expect(!codec.fromPacket(invalidSlot).has_value(), "inventory command codec should reject invalid equipment slot");

	dev::InventoryCommandPacket extraPayload {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		.hasItemId = 1,
		.itemId = 961,
		.hasSlot = 1,
		.slot = static_cast<uint8_t>(dev::EquipmentSlot::Weapon),
	};
	Expect(!codec.fromPacket(extraPayload).has_value(), "inventory command codec should reject unexpected payload fields");

	dev::InventoryCommandPacket invalidBoolean {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		.hasItemId = 2,
		.itemId = 961,
	};
	Expect(!codec.fromPacket(invalidBoolean).has_value(), "inventory command codec should reject invalid payload flags");

	dev::InventoryCommandBytes shortBytes = codec.encode(codec.toPacket({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Armor,
	}));
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "inventory command codec should reject wrong byte size");
}

void TestInventoryCommandPacketValidatorRejectsMalformedPayloads()
{
	dev::InventoryCommandPacketValidator validator;

	Expect(validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
	           .hasItemId = 1,
	           .itemId = 961,
	       }),
	    "inventory command packet validator should accept valid equip packets");
	Expect(validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
	           .hasSlot = 1,
	           .slot = static_cast<uint8_t>(dev::EquipmentSlot::Weapon),
	       }),
	    "inventory command packet validator should accept valid unequip packets");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
	           .hasItemId = 2,
	           .itemId = 961,
	       }),
	    "inventory command packet validator should reject non-boolean payload flags");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
	           .hasSlot = 1,
	           .slot = 99,
	       }),
	    "inventory command packet validator should reject invalid equipment slots");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
	           .hasItemId = 1,
	           .itemId = 961,
	           .hasSlot = 1,
	           .slot = static_cast<uint8_t>(dev::EquipmentSlot::Weapon),
	       }),
	    "inventory command packet validator should reject unexpected payload fields");
}

void TestInventoryCommandByteStreamWritesLittleEndianPrimitives()
{
	dev::InventoryCommandBytes bytes;
	dev::InventoryCommandByteWriter writer { bytes };
	writer.writeU8(0xAB);
	writer.writeU16(0x1234);
	writer.writeU32(0x01020304);

	Expect(bytes == dev::InventoryCommandBytes({ 0xAB, 0x34, 0x12, 0x04, 0x03, 0x02, 0x01 }), "inventory command byte stream should write little-endian primitives");

	dev::InventoryCommandByteReader reader { bytes };
	uint8_t one = 0;
	uint16_t two = 0;
	uint32_t four = 0;
	Expect(reader.readU8(one), "inventory command byte stream should read u8");
	Expect(reader.readU16(two), "inventory command byte stream should read u16");
	Expect(reader.readU32(four), "inventory command byte stream should read u32");
	Expect(one == 0xAB && two == 0x1234 && four == 0x01020304, "inventory command byte stream should preserve primitive values");
	Expect(reader.consumed(), "inventory command byte stream should track consumed bytes");
}

void TestInventoryCommandByteStreamRejectsShortReads()
{
	dev::InventoryCommandBytes bytes { 1, 2, 3 };
	dev::InventoryCommandByteReader reader { bytes };
	uint32_t value = 0;

	Expect(!reader.readU32(value), "inventory command byte stream should reject short u32 reads");
	Expect(reader.offset() == 0, "inventory command byte stream should not advance offset after rejected read");
}

void TestInventoryCommandPacketByteCodecRoundTripsPackets()
{
	dev::InventoryCommandPacketByteCodec codec;
	dev::InventoryCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		.hasItemId = 1,
		.itemId = 0x01020304U,
	};

	dev::InventoryCommandBytes bytes = codec.encode(packet);
	std::optional<dev::InventoryCommandPacket> decoded = codec.decode(bytes);

	Expect(bytes.size() == 16, "inventory command packet byte codec should write fixed packet size");
	Expect(bytes.size() == 16 && bytes[2] == 0x04 && bytes[3] == 0x03 && bytes[4] == 0x02 && bytes[5] == 0x01, "inventory command packet byte codec should write item id little-endian");
	Expect(decoded.has_value(), "inventory command packet byte codec should decode valid bytes");
	Expect(decoded.has_value() && decoded->commandType == packet.commandType, "inventory command packet byte codec should preserve command type");
	Expect(decoded.has_value() && decoded->hasItemId == 1 && decoded->itemId == packet.itemId, "inventory command packet byte codec should preserve item payload");
}

void TestInventoryCommandPacketByteCodecRejectsInvalidBytes()
{
	dev::InventoryCommandPacketByteCodec codec;
	dev::InventoryCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
		.hasSlot = 1,
		.slot = static_cast<uint8_t>(dev::EquipmentSlot::Weapon),
	};

	dev::InventoryCommandBytes shortBytes = codec.encode(packet);
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "inventory command packet byte codec should reject wrong byte size");

	dev::InventoryCommandBytes invalidPacketBytes = codec.encode(packet);
	invalidPacketBytes[7] = 99;
	Expect(!codec.decode(invalidPacketBytes).has_value(), "inventory command packet byte codec should reject invalid decoded packets");
}

void TestInventoryCommandLogReplaysThroughDispatcher()
{
	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 962,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryCommandReplayer replayer { dispatcher };
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 962,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	std::vector<dev::InventoryCommandResult> results = replayer.replay(log);

	Expect(results.size() == 2, "inventory command log should replay every command");
	Expect(results.size() == 2 && results[0].type == dev::InventoryCommandResultType::Applied, "inventory command replay should apply equip command");
	Expect(results.size() == 2 && results[1].type == dev::InventoryCommandResultType::Applied, "inventory command replay should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "inventory command replay should leave weapon slot empty after unequip");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 962, "inventory command replay should return unequipped item to bag");
	Expect(events.events().size() == 2, "inventory command replay should emit inventory events");
	Expect(events.events().size() == 2 && events.events()[0].type == dev::InventoryEventType::Equipped, "inventory command replay should emit equipped event");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::InventoryEventType::Unequipped, "inventory command replay should emit unequipped event");
}

void TestInventoryCommandLogCodecRoundTripsAndReplays()
{
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 963,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogCodec codec;
	dev::InventoryCommandLogBytes bytes = codec.encode(log);
	std::optional<dev::InventoryCommandLog> decoded = codec.decode(bytes);
	Expect(decoded.has_value(), "inventory command log codec should decode its own bytes");
	Expect(decoded.has_value() && decoded->commands().size() == 2, "inventory command log codec should preserve command count");
	Expect(decoded.has_value() && decoded->commands()[0].itemId == std::optional<dev::TargetId> { 963 }, "inventory command log codec should preserve equip item id");
	Expect(decoded.has_value() && decoded->commands()[1].slot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Weapon }, "inventory command log codec should preserve unequip slot");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 963,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryCommandDispatcher dispatcher { player };
	dev::InventoryCommandReplayer replayer { dispatcher };
	std::vector<dev::InventoryCommandResult> results = decoded.has_value()
	    ? replayer.replay(*decoded)
	    : std::vector<dev::InventoryCommandResult> {};

	Expect(results.size() == 2, "decoded inventory command log should replay");
	Expect(!player.inventory.equipment.weapon.has_value(), "decoded inventory command log should reproduce inventory state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 963, "decoded inventory command log should preserve item ownership");
}

void TestInventoryCommandLogCodecRejectsInvalidBytes()
{
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 964,
	});

	dev::InventoryCommandLogCodec codec;
	dev::InventoryCommandLogBytes bytes = codec.encode(log);

	dev::InventoryCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "inventory command log codec should reject bad magic");

	dev::InventoryCommandLogBytes badVersion = bytes;
	badVersion[4] = 2;
	Expect(!codec.decode(badVersion).has_value(), "inventory command log codec should reject bad version");

	dev::InventoryCommandLogBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "inventory command log codec should reject truncated bytes");

	dev::InventoryCommandLogBytes corrupted = bytes;
	corrupted[12] ^= 0x01U;
	Expect(!codec.decode(corrupted).has_value(), "inventory command log codec should reject checksum mismatch");
}

void TestInventoryCommandLogChecksumValidatesTrailingChecksum()
{
	dev::InventoryCommandLogBytes bytes { 1, 2, 3, 4 };
	dev::InventoryCommandLogChecksum checksum;
	const uint32_t expected = checksum.compute(bytes, bytes.size());

	checksum.appendTo(bytes);

	Expect(bytes.size() == 8, "inventory command log checksum should append four checksum bytes");
	Expect(checksum.hasValidTrailingChecksum(bytes, 4), "inventory command log checksum should validate appended checksum");
	Expect(expected == checksum.compute(bytes, 4), "inventory command log checksum should compute payload hash only");

	bytes[0] ^= 0xFFU;
	Expect(!checksum.hasValidTrailingChecksum(bytes, 4), "inventory command log checksum should reject mutated payload");
}

void TestInventoryCommandPacketListCodecFramesPacketBytes()
{
	dev::InventoryCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		.hasItemId = 1,
		.itemId = 100,
	};
	dev::InventoryCommandPacketByteCodec packetCodec;
	dev::InventoryCommandBytes packetBytes = packetCodec.encode(packet);

	dev::InventoryCommandLogBytes bytes = dev::InventoryCommandPacketListCodec {}.encode({ packetBytes, packetBytes });
	std::optional<std::vector<dev::InventoryCommandBytes>> decoded = dev::InventoryCommandPacketListCodec {}.decode(bytes);

	Expect(decoded.has_value(), "inventory command packet list codec should decode encoded packet lists");
	Expect(decoded.has_value() && decoded->size() == 2, "inventory command packet list codec should preserve packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packetBytes, "inventory command packet list codec should preserve first packet bytes");
	Expect(decoded.has_value() && (*decoded)[1] == packetBytes, "inventory command packet list codec should preserve second packet bytes");
}

void TestInventoryCommandPacketListCodecRejectsInvalidSizes()
{
	dev::InventoryCommandLogBytes missingCount { 1, 2 };
	dev::InventoryCommandLogBytes wrongSize {
		1, 0, 0, 0,
		1, 2, 3,
	};

	dev::InventoryCommandPacketListCodec codec;
	Expect(!codec.decode(missingCount).has_value(), "inventory command packet list codec should reject missing command count");
	Expect(!codec.decode(wrongSize).has_value(), "inventory command packet list codec should reject packet lists with invalid size");
}

void TestInventoryCommandLogFrameCodecFramesPacketBytes()
{
	dev::InventoryCommandPacketByteCodec packetCodec;
	std::vector<dev::InventoryCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		    .hasItemId = 1,
		    .itemId = 962,
		}),
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
		    .hasSlot = 1,
		    .slot = static_cast<uint8_t>(dev::EquipmentSlot::Weapon),
		}),
	};

	dev::InventoryCommandLogFrameCodec frameCodec;
	dev::InventoryCommandLogBytes bytes = frameCodec.encode(packets);
	std::optional<std::vector<dev::InventoryCommandBytes>> decoded = frameCodec.decode(bytes);

	Expect(bytes.size() == 48, "inventory command log frame codec should write header, packets, and checksum");
	Expect(bytes.size() == 48 && bytes[0] == 'I' && bytes[1] == 'I' && bytes[2] == 'C' && bytes[3] == 'L', "inventory command log frame codec should write magic");
	Expect(bytes.size() == 48 && bytes[4] == 1 && bytes[8] == 2, "inventory command log frame codec should write version and command count");
	Expect(decoded.has_value() && decoded->size() == 2, "inventory command log frame codec should restore packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packets[0], "inventory command log frame codec should preserve first packet");
	Expect(decoded.has_value() && (*decoded)[1] == packets[1], "inventory command log frame codec should preserve second packet");
}

void TestInventoryCommandLogFrameCodecRejectsInvalidFrames()
{
	dev::InventoryCommandPacketByteCodec packetCodec;
	std::vector<dev::InventoryCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		    .hasItemId = 1,
		    .itemId = 962,
		}),
	};
	dev::InventoryCommandLogFrameCodec frameCodec;
	dev::InventoryCommandLogBytes bytes = frameCodec.encode(packets);

	dev::InventoryCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!frameCodec.decode(badMagic).has_value(), "inventory command log frame codec should reject checksum-protected bad magic");

	dev::InventoryCommandLogBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 2;
	dev::InventoryCommandLogChecksum {}.appendTo(badVersion);
	Expect(!frameCodec.decode(badVersion).has_value(), "inventory command log frame codec should reject unsupported version");

	dev::InventoryCommandLogBytes wrongCount = bytes;
	wrongCount.resize(wrongCount.size() - 4U);
	wrongCount[8] = 2;
	dev::InventoryCommandLogChecksum {}.appendTo(wrongCount);
	Expect(!frameCodec.decode(wrongCount).has_value(), "inventory command log frame codec should reject payload size mismatch");
}

void TestByteFileStoreSavesLoadsAndCleansTempFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_byte_file_store_test";
	const std::filesystem::path path = root / "bytes.bin";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::ByteFileStore store;
	std::vector<uint8_t> bytes { 0, 1, 2, 255 };

	Expect(store.save(path, bytes), "byte file store should save binary bytes");
	std::optional<std::vector<uint8_t>> loaded = store.load(path);

	Expect(loaded.has_value(), "byte file store should load saved bytes");
	Expect(loaded.has_value() && *loaded == bytes, "byte file store should preserve binary byte payload");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "byte file store should remove temp file after save");

	std::filesystem::remove_all(root);
}

void TestByteFileStoreRejectsMissingAndUnwritablePaths()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_byte_file_store_missing_test";
	const std::filesystem::path missing = root / "missing.bin";
	const std::filesystem::path unwritable = root / "missing_directory" / "bytes.bin";
	std::filesystem::remove_all(root);

	dev::ByteFileStore store;

	Expect(!store.load(missing).has_value(), "byte file store should reject missing files");
	Expect(!store.save(unwritable, { 1, 2, 3 }), "byte file store should reject saves when parent directory is missing");
	Expect(!std::filesystem::exists(unwritable.string() + ".tmp"), "byte file store should not leave temp files after failed open");

	std::filesystem::remove_all(root);
}

void TestTextFileStoreSavesLoadsAndCleansTempFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_text_file_store_test";
	const std::filesystem::path path = root / "lines.txt";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::TextFileStore store;
	std::vector<std::string> lines {
		"run frames=1 frameReports=1",
		"frame[0]",
		"inventoryResult[0] type=Applied",
	};

	Expect(store.saveLines(path, lines), "text file store should save text lines");
	std::optional<std::vector<std::string>> loaded = store.loadLines(path);

	Expect(loaded.has_value(), "text file store should load saved lines");
	Expect(loaded.has_value() && *loaded == lines, "text file store should preserve line payloads");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "text file store should remove temp file after save");

	std::filesystem::remove_all(root);
}

void TestTextFileStoreRejectsMissingAndUnwritablePaths()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_text_file_store_missing_test";
	const std::filesystem::path missing = root / "missing.txt";
	const std::filesystem::path unwritable = root / "missing_directory" / "lines.txt";
	std::filesystem::remove_all(root);

	dev::TextFileStore store;

	Expect(!store.loadLines(missing).has_value(), "text file store should reject missing files");
	Expect(!store.saveLines(unwritable, { "line" }), "text file store should reject saves when parent directory is missing");
	Expect(!std::filesystem::exists(unwritable.string() + ".tmp"), "text file store should not leave temp files after failed open");

	std::filesystem::remove_all(root);
}

void TestInventoryCommandLogFileStoreSavesLoadsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_log_file_store_replay_test";
	const std::filesystem::path path = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 965,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory command log file store should save log");
	std::optional<dev::InventoryCommandLog> loaded = store.load(path);

	Expect(loaded.has_value(), "inventory command log file store should load saved log");
	Expect(loaded.has_value() && loaded->commands().size() == 2, "inventory command log file store should preserve command count");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "inventory command log file store should remove temp file after save");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 965,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryCommandReplayer replayer { dispatcher };
	std::vector<dev::InventoryCommandResult> results = loaded.has_value()
	    ? replayer.replay(*loaded)
	    : std::vector<dev::InventoryCommandResult> {};

	Expect(results.size() == 2, "loaded inventory command log should replay");
	Expect(!player.inventory.equipment.weapon.has_value(), "loaded inventory command log should reproduce inventory state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 965, "loaded inventory command log should preserve item ownership");
	Expect(events.events().size() == 2, "loaded inventory command log replay should emit events");

	std::filesystem::remove_all(root);
}

void TestInventoryCommandLogFileStoreRejectsCorruptAndMissingFiles()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_log_file_store_corrupt_test";
	const std::filesystem::path missingPath = root / "missing.iicl";
	const std::filesystem::path corruptPath = root / "corrupt.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	{
		std::ofstream output { corruptPath, std::ios::binary | std::ios::trunc };
		output << "not an inventory command log";
	}

	dev::InventoryCommandLogFileStore store;
	Expect(!store.load(missingPath).has_value(), "inventory command log file store should reject missing file");
	Expect(!store.load(corruptPath).has_value(), "inventory command log file store should reject corrupt file");

	std::filesystem::remove_all(root);
}

void TestInventoryScriptRunnerRunsSavedInventoryScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_script_runner_test";
	const std::filesystem::path path = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 966,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory script runner test should create script file");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 966,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryScriptRunner runner { dispatcher };
	dev::InventoryScriptRunResult result = runner.run(path);

	Expect(result.status == dev::InventoryScriptRunStatus::Completed, "inventory script runner should complete valid script files");
	Expect(result.commandResults.size() == 2, "inventory script runner should return per-command results");
	Expect(result.commandResults.size() == 2 && result.commandResults[0].type == dev::InventoryCommandResultType::Applied, "inventory script runner should apply equip command");
	Expect(result.commandResults.size() == 2 && result.commandResults[1].type == dev::InventoryCommandResultType::Applied, "inventory script runner should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "inventory script runner should reproduce unequipped state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 966, "inventory script runner should preserve item ownership");
	Expect(events.events().size() == 2, "inventory script runner should still emit dispatcher events");

	std::filesystem::remove_all(root);
}

void TestInventoryScriptRunnerReportsLoadFailureAndCommandRejectionSeparately()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_script_runner_failure_test";
	const std::filesystem::path path = root / "inventory.iicl";
	const std::filesystem::path missingPath = root / "missing.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::Player player = MakePlayer();
	player.inventory.items.push_back({ .id = 967 });
	dev::InventoryCommandDispatcher dispatcher { player };
	dev::InventoryScriptRunner runner { dispatcher };

	dev::InventoryScriptRunResult missing = runner.run(missingPath);
	Expect(missing.status == dev::InventoryScriptRunStatus::LoadFailed, "inventory script runner should report missing file load failure");
	Expect(missing.commandResults.empty(), "missing inventory script should not dispatch commands");

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 967,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory script runner rejection test should create script file");

	dev::InventoryScriptRunResult rejected = runner.run(path);
	Expect(rejected.status == dev::InventoryScriptRunStatus::Completed, "inventory script runner should complete loadable scripts even when commands reject");
	Expect(rejected.commandResults.size() == 1, "inventory script runner should return rejected command result");
	Expect(rejected.commandResults.size() == 1 && rejected.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "inventory script runner should preserve command-level rejection");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 967, "rejected inventory script command should not mutate inventory");

	std::filesystem::remove_all(root);
}

void TestSimulationSnapshotRestoresDurableState()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	world.movementEvents = &movementEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 2, 2 }));
	world.players[0].combatStats.hitPoints = 18;
	world.players[0].inventory.capacity = 3;
	world.players[0].inventory.items.push_back({ .id = 52, .tile = { 0, 0 }, .combatModifiers = { .defense = 1 } });
	world.players[0].inventory.equipment.weapon = dev::Item { .id = 53, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 2 } };
	world.enemies.push_back(MakeEnemy({ 4, 4 }));
	world.enemies[0].moveState = dev::EnemyMoveState::Attacking;
	world.enemies[0].stateTimerSeconds = 0.50F;
	world.items.push_back({ .id = 51, .tile = { 3, 2 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 50, .tile = { 4, 4 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 7, .attackPower = 3, .defense = 1 },
	});
	world.targets.add(target);
	world.targets.add({ .type = dev::TargetType::Item, .id = 51, .tile = { 3, 2 } });
	movementEvents.emit({ .type = dev::MovementEventType::StepCommitted, .tile = { 2, 2 } });
	combatEvents.emit({ .type = dev::CombatEventType::Hit, .target = target, .damage = 2, .remainingHitPoints = 7 });

	dev::SimulationSnapshot snapshot = dev::SnapshotWriter {}.write(world);

	world.players[0].position.tile = { 9, 9 };
	world.players[0].combatStats.hitPoints = 1;
	world.players[0].inventory.items.clear();
	world.enemies.clear();
	world.items.clear();
	world.combat.registry().replaceAll({});
	world.targets.clear();
	dev::SnapshotReader {}.read(snapshot, world);

	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target restoredTarget = world.targets.resolveAtTile({ 4, 4 });
	dev::Target restoredItem = world.targets.resolveAtTile({ 3, 2 });
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 2, 2 }, "snapshot should restore player position");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 18, "snapshot should restore player combat stats");
	Expect(world.players.size() == 1 && world.players[0].inventory.capacity == 3, "snapshot should restore player inventory capacity");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 52, "snapshot should restore player inventory");
	Expect(world.players.size() == 1 && world.players[0].inventory.equipment.weapon.has_value() && world.players[0].inventory.equipment.weapon->id == 53, "snapshot should restore player equipment");
	Expect(world.players.size() == 1 && world.players[0].inventory.equipment.weapon.has_value() && world.players[0].inventory.equipment.weapon->combatModifiers.attackPower == 2, "snapshot should restore equipment combat modifiers");
	Expect(world.enemies.size() == 1 && world.enemies[0].position.tile == dev::Point { 4, 4 }, "snapshot should restore enemy position");
	Expect(world.enemies.size() == 1 && world.enemies[0].moveState == dev::EnemyMoveState::Attacking, "snapshot should restore enemy state");
	Expect(world.items.size() == 1 && world.items[0].tile == dev::Point { 3, 2 }, "snapshot should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 7, "snapshot should restore combat registry state");
	Expect(restoredTarget.type == dev::TargetType::Enemy && restoredTarget.id == 50, "snapshot should restore target registry enemy");
	Expect(restoredItem.type == dev::TargetType::Item && restoredItem.id == 51, "snapshot should restore target registry item");
	Expect(snapshot.players.size() == 1 && snapshot.enemies.size() == 1 && snapshot.items.size() == 1 && snapshot.combatants.size() == 1 && snapshot.targets.size() == 2, "snapshot should contain durable state only");
	Expect(!movementEvents.events().empty() && !combatEvents.events().empty(), "snapshot restore should not manage transient event history");
}

void TestSnapshotCodecRoundTripsVersionedBytes()
{
	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 1, 2 });
	player.moveState = dev::PlayerMoveState::Pathing;
	player.path.pushStep({ 2, 2 });
	player.path.pushStep({ 3, 2 });
	player.destinationAction = {
	    dev::DestinationActionType::Attack,
	    { .type = dev::TargetType::Enemy, .id = 60, .tile = { 3, 2 } },
	    1,
	};
	player.movementModifiers.standGround = true;
	player.animationLock.active = true;
	player.animationLock.elapsedSeconds = 0.25F;
	player.animationLock.cancelAfterSeconds = 0.50F;
	player.combatStats.hitPoints = 11;
	player.inventory.capacity = 2;
	player.inventory.items.push_back({ .id = 62, .tile = { 0, 0 }, .combatModifiers = { .defense = 3 } });
	player.inventory.equipment.weapon = dev::Item { .id = 63, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 4 } };
	snapshot.players.push_back(player);

	dev::Enemy enemy = MakeEnemy({ 5, 5 });
	enemy.id = 60;
	enemy.moveState = dev::EnemyMoveState::Recovering;
	enemy.tuning.attackWindupSeconds = 0.75F;
	enemy.stateTimerSeconds = 0.25F;
	snapshot.enemies.push_back(enemy);
	snapshot.items.push_back({ .id = 61, .tile = { 6, 5 }, .equipmentSlot = dev::EquipmentSlot::Accessory, .combatModifiers = { .attackPower = 1, .defense = 1 } });
	snapshot.combatants.push_back({
	    .target = { .type = dev::TargetType::Enemy, .id = 60, .tile = { 5, 5 } },
	    .stats = { .hitPoints = 4, .attackPower = 7, .defense = 2 },
	});
	snapshot.targets.push_back({ .type = dev::TargetType::Enemy, .id = 60, .tile = { 5, 5 } });
	snapshot.targets.push_back({ .type = dev::TargetType::Object, .id = 61, .tile = { 6, 5 } });

	dev::SnapshotCodec codec;
	dev::SnapshotBytes bytes = codec.encode(snapshot);
	std::optional<dev::SimulationSnapshot> decoded = codec.decode(bytes);

	Expect(decoded.has_value(), "snapshot codec should decode its own bytes");
	if (!decoded.has_value())
		return;

	Expect(decoded->players.size() == 1, "snapshot codec should preserve player count");
	Expect(decoded->players.size() == 1 && decoded->players[0].position.tile == dev::Point { 1, 2 }, "snapshot codec should preserve player position");
	Expect(decoded->players.size() == 1 && decoded->players[0].path.size() == 2, "snapshot codec should preserve player path length");
	Expect(decoded->players.size() == 1 && decoded->players[0].path.peekNext() == std::optional<dev::Point> { { 2, 2 } }, "snapshot codec should preserve next path step");
	Expect(decoded->players.size() == 1 && decoded->players[0].destinationAction.target.id == 60, "snapshot codec should preserve destination action target");
	Expect(decoded->players.size() == 1 && decoded->players[0].animationLock.active, "snapshot codec should preserve animation lock");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.capacity == 2, "snapshot codec should preserve player inventory capacity");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.items.size() == 1 && decoded->players[0].inventory.items[0].id == 62, "snapshot codec should preserve player inventory");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.items.size() == 1 && decoded->players[0].inventory.items[0].combatModifiers.defense == 3, "snapshot codec should preserve inventory item combat modifiers");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.equipment.weapon.has_value() && decoded->players[0].inventory.equipment.weapon->id == 63, "snapshot codec should preserve player equipment");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.equipment.weapon.has_value() && decoded->players[0].inventory.equipment.weapon->combatModifiers.attackPower == 4, "snapshot codec should preserve equipment combat modifiers");
	Expect(decoded->enemies.size() == 1 && decoded->enemies[0].moveState == dev::EnemyMoveState::Recovering, "snapshot codec should preserve enemy state");
	Expect(decoded->items.size() == 1 && decoded->items[0].tile == dev::Point { 6, 5 }, "snapshot codec should preserve item state");
	Expect(decoded->items.size() == 1 && decoded->items[0].equipmentSlot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Accessory }, "snapshot codec should preserve floor item equipment slot");
	Expect(decoded->items.size() == 1 && decoded->items[0].combatModifiers.attackPower == 1, "snapshot codec should preserve floor item combat modifiers");
	Expect(decoded->combatants.size() == 1 && decoded->combatants[0].stats.hitPoints == 4, "snapshot codec should preserve combatants");
	Expect(decoded->targets.size() == 2 && decoded->targets[0].type == dev::TargetType::Enemy, "snapshot codec should preserve target registry target type");
	Expect(decoded->targets.size() == 2 && decoded->targets[1].id == 61, "snapshot codec should preserve target registry target id");
}

void TestSnapshotCodecRejectsInvalidBytes()
{
	dev::SnapshotCodec codec;
	dev::SimulationSnapshot snapshot;
	snapshot.players.push_back(MakePlayer());
	dev::SnapshotBytes bytes = codec.encode(snapshot);

	dev::SnapshotBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "snapshot codec should reject bad magic");

	dev::SnapshotBytes badVersion = bytes;
	badVersion[4] = 8;
	Expect(!codec.decode(badVersion).has_value(), "snapshot codec should reject unsupported version");

	dev::SnapshotBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "snapshot codec should reject truncated data");

	dev::SnapshotBytes corruptedPayload = bytes;
	corruptedPayload[12] ^= 0x01U;
	Expect(!codec.decode(corruptedPayload).has_value(), "snapshot codec should reject checksum mismatch");
}

void TestSnapshotChecksumValidatesTrailingChecksum()
{
	dev::SnapshotBytes bytes { 1, 2, 3, 4 };
	dev::SnapshotChecksum checksum;
	const uint32_t expected = checksum.compute(bytes, bytes.size());

	checksum.appendTo(bytes);

	Expect(bytes.size() == 8, "snapshot checksum should append four checksum bytes");
	Expect(checksum.hasValidTrailingChecksum(bytes, 4), "snapshot checksum should validate appended checksum");
	Expect(expected == checksum.compute(bytes, 4), "snapshot checksum should compute payload hash only");

	bytes[0] ^= 0xFFU;
	Expect(!checksum.hasValidTrailingChecksum(bytes, 4), "snapshot checksum should reject mutated payload");
}

void TestSnapshotByteStreamWritesLittleEndianPrimitives()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	writer.writeU8(0xABU);
	writer.writeU32(0x12345678U);
	writer.writeI32(-2);
	writer.writeFloat(1.5F);

	Expect(bytes.size() == 13, "snapshot byte writer should append primitive bytes");
	Expect(bytes.size() == 13 && bytes[1] == 0x78U && bytes[2] == 0x56U && bytes[3] == 0x34U && bytes[4] == 0x12U, "snapshot byte writer should write uint32 little-endian");

	dev::SnapshotByteReader reader { bytes };
	uint8_t byte = 0;
	uint32_t unsignedValue = 0;
	int signedValue = 0;
	float floatValue = 0.0F;
	Expect(reader.readU8(byte) && byte == 0xABU, "snapshot byte reader should read u8");
	Expect(reader.readU32(unsignedValue) && unsignedValue == 0x12345678U, "snapshot byte reader should read u32");
	Expect(reader.readI32(signedValue) && signedValue == -2, "snapshot byte reader should read i32");
	Expect(reader.readFloat(floatValue) && Near(floatValue, 1.5F), "snapshot byte reader should read float");
	Expect(reader.consumed(), "snapshot byte reader should report consumed bytes");

	dev::SnapshotByteReader offsetReader { bytes, 1 };
	Expect(offsetReader.readU32(unsignedValue) && unsignedValue == 0x12345678U, "snapshot byte reader should read from a starting offset");
}

void TestSnapshotByteStreamRejectsShortReads()
{
	dev::SnapshotBytes bytes { 1, 2, 3 };
	dev::SnapshotByteReader reader { bytes };
	uint32_t value = 0;
	uint8_t first = 0;

	Expect(!reader.readU32(value), "snapshot byte reader should reject short u32 reads");
	Expect(reader.offset() == 0, "snapshot byte reader should not advance after failed reads");
	Expect(reader.readU8(first) && first == 1, "snapshot byte reader should continue after failed reads");
}

void TestSnapshotEntityCodecRoundTripsItemAndCombatant()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEntityCodec codec;
	dev::Item item {
	    .id = 91,
	    .tile = { 3, 4 },
	    .equipmentSlot = dev::EquipmentSlot::Accessory,
	    .combatModifiers = { .attackPower = 2, .defense = 5 },
	};
	dev::Combatant combatant {
	    .target = { .type = dev::TargetType::Enemy, .id = 92, .tile = { 5, 6 } },
	    .stats = { .hitPoints = 7, .attackPower = 8, .defense = 9 },
	};
	codec.writeItem(writer, item);
	codec.writeCombatant(writer, combatant);

	dev::SnapshotByteReader reader { bytes };
	dev::Item decodedItem;
	dev::Combatant decodedCombatant;
	Expect(codec.readItem(reader, decodedItem), "snapshot entity codec should read encoded item");
	Expect(codec.readCombatant(reader, decodedCombatant), "snapshot entity codec should read encoded combatant");
	Expect(decodedItem.id == 91 && decodedItem.tile == dev::Point { 3, 4 }, "snapshot entity codec should preserve item identity and tile");
	Expect(decodedItem.equipmentSlot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Accessory }, "snapshot entity codec should preserve item equipment slot");
	Expect(decodedItem.combatModifiers.attackPower == 2 && decodedItem.combatModifiers.defense == 5, "snapshot entity codec should preserve item combat modifiers");
	Expect(decodedCombatant.target.type == dev::TargetType::Enemy && decodedCombatant.target.id == 92 && decodedCombatant.target.tile == dev::Point { 5, 6 }, "snapshot entity codec should preserve combatant target");
	Expect(decodedCombatant.stats.hitPoints == 7 && decodedCombatant.stats.attackPower == 8 && decodedCombatant.stats.defense == 9, "snapshot entity codec should preserve combatant stats");
	Expect(reader.consumed(), "snapshot entity codec should consume encoded entity bytes");
}

void TestSnapshotEntityCodecRejectsInvalidEnums()
{
	dev::SnapshotEntityCodec codec;

	dev::SnapshotBytes badTargetBytes;
	dev::SnapshotByteWriter badTargetWriter { badTargetBytes };
	badTargetWriter.writeU8(static_cast<uint8_t>(dev::TargetType::Object) + 1U);
	badTargetWriter.writeU32(1);
	badTargetWriter.writeI32(0);
	badTargetWriter.writeI32(0);
	dev::SnapshotByteReader badTargetReader { badTargetBytes };
	dev::Target target;
	Expect(!codec.readTarget(badTargetReader, target), "snapshot entity codec should reject invalid target type");

	dev::SnapshotBytes badSlotBytes;
	dev::SnapshotByteWriter badSlotWriter { badSlotBytes };
	badSlotWriter.writeU32(2);
	badSlotWriter.writeI32(1);
	badSlotWriter.writeI32(1);
	badSlotWriter.writeU8(1);
	badSlotWriter.writeU8(static_cast<uint8_t>(dev::EquipmentSlot::Accessory) + 1U);
	badSlotWriter.writeI32(0);
	badSlotWriter.writeI32(0);
	dev::SnapshotByteReader badSlotReader { badSlotBytes };
	dev::Item item;
	Expect(!codec.readItem(badSlotReader, item), "snapshot entity codec should reject invalid equipment slot");

	dev::SnapshotBytes badActionBytes;
	dev::SnapshotByteWriter badActionWriter { badActionBytes };
	badActionWriter.writeU8(static_cast<uint8_t>(dev::DestinationActionType::Interact) + 1U);
	dev::SnapshotByteReader badActionReader { badActionBytes };
	dev::DestinationAction action;
	Expect(!codec.readDestinationAction(badActionReader, action), "snapshot entity codec should reject invalid destination action type");
}

void TestSnapshotPlayerCodecRoundTripsDurablePlayerState()
{
	dev::Player player = MakePlayer({ 4, 5 });
	player.position.future = { 5, 5 };
	player.position.previous = { 3, 5 };
	player.position.precise = { 4, 5 };
	player.moveState = dev::PlayerMoveState::Pathing;
	player.path.pushStep({ 5, 5 });
	player.path.pushStep({ 6, 5 });
	player.destinationAction = {
	    dev::DestinationActionType::Attack,
	    { .type = dev::TargetType::Enemy, .id = 72, .tile = { 6, 5 } },
	    1,
	};
	player.movementModifiers.standGround = true;
	player.animationLock.active = true;
	player.animationLock.elapsedSeconds = 0.25F;
	player.animationLock.cancelAfterSeconds = 0.75F;
	player.combatStats = { .hitPoints = 13, .attackPower = 8, .defense = 4 };
	player.inventory.capacity = 4;
	player.inventory.items.push_back({ .id = 73, .tile = { 2, 2 }, .combatModifiers = { .defense = 2 } });
	player.inventory.equipment.weapon = dev::Item { .id = 74, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 5 } };
	player.inventory.equipment.armor = dev::Item { .id = 75, .equipmentSlot = dev::EquipmentSlot::Armor, .combatModifiers = { .defense = 6 } };
	player.moveSpeedTilesPerSecond = 6.5F;

	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotPlayerCodec codec;
	codec.writePlayer(writer, player);

	dev::SnapshotByteReader reader { bytes };
	dev::Player decoded;
	Expect(codec.readPlayer(reader, decoded), "snapshot player codec should read encoded player");
	Expect(decoded.position.tile == dev::Point { 4, 5 } && decoded.position.future == dev::Point { 5, 5 }, "snapshot player codec should preserve actor position");
	Expect(decoded.moveState == dev::PlayerMoveState::Pathing, "snapshot player codec should preserve move state");
	Expect(decoded.path.size() == 2 && decoded.path.peekNext() == std::optional<dev::Point> { { 5, 5 } }, "snapshot player codec should preserve path steps");
	Expect(decoded.destinationAction.type == dev::DestinationActionType::Attack && decoded.destinationAction.target.id == 72, "snapshot player codec should preserve destination action");
	Expect(decoded.movementModifiers.standGround, "snapshot player codec should preserve movement modifiers");
	Expect(decoded.animationLock.active && Near(decoded.animationLock.elapsedSeconds, 0.25F) && Near(decoded.animationLock.cancelAfterSeconds, 0.75F), "snapshot player codec should preserve animation lock");
	Expect(decoded.combatStats.hitPoints == 13 && decoded.combatStats.attackPower == 8 && decoded.combatStats.defense == 4, "snapshot player codec should preserve combat stats");
	Expect(decoded.inventory.capacity == 4 && decoded.inventory.items.size() == 1 && decoded.inventory.items[0].id == 73, "snapshot player codec should preserve inventory contents");
	Expect(decoded.inventory.equipment.weapon.has_value() && decoded.inventory.equipment.weapon->combatModifiers.attackPower == 5, "snapshot player codec should preserve weapon equipment");
	Expect(decoded.inventory.equipment.armor.has_value() && decoded.inventory.equipment.armor->combatModifiers.defense == 6, "snapshot player codec should preserve armor equipment");
	Expect(Near(decoded.moveSpeedTilesPerSecond, 6.5F), "snapshot player codec should preserve movement speed");
	Expect(reader.consumed(), "snapshot player codec should consume encoded player bytes");
}

void TestSnapshotPlayerCodecRejectsInvalidMoveState()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEntityCodec entityCodec;
	entityCodec.writeActorPosition(writer, dev::ActorPosition {});
	writer.writeU8(static_cast<uint8_t>(dev::PlayerMoveState::Acting) + 1U);

	dev::SnapshotByteReader reader { bytes };
	dev::Player player;
	Expect(!dev::SnapshotPlayerCodec {}.readPlayer(reader, player), "snapshot player codec should reject invalid move state");
}

void TestSnapshotEnemyCodecRoundTripsDurableEnemyState()
{
	dev::Enemy enemy = MakeEnemy({ 7, 8 });
	enemy.id = 81;
	enemy.position.future = { 8, 8 };
	enemy.position.previous = { 6, 8 };
	enemy.position.precise = { 7, 8 };
	enemy.moveState = dev::EnemyMoveState::Recovering;
	enemy.tuning.maxStepsPerTick = 2;
	enemy.tuning.attackRangeTiles = 3;
	enemy.tuning.attackWindupSeconds = 0.60F;
	enemy.tuning.attackRecoverySeconds = 0.90F;
	enemy.combatStats = { .hitPoints = 10, .attackPower = 11, .defense = 12 };
	enemy.stateTimerSeconds = 1.25F;

	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEnemyCodec codec;
	codec.writeEnemy(writer, enemy);

	dev::SnapshotByteReader reader { bytes };
	dev::Enemy decoded;
	Expect(codec.readEnemy(reader, decoded), "snapshot enemy codec should read encoded enemy");
	Expect(decoded.id == 81, "snapshot enemy codec should preserve enemy id");
	Expect(decoded.position.tile == dev::Point { 7, 8 } && decoded.position.future == dev::Point { 8, 8 }, "snapshot enemy codec should preserve actor position");
	Expect(decoded.moveState == dev::EnemyMoveState::Recovering, "snapshot enemy codec should preserve move state");
	Expect(decoded.tuning.maxStepsPerTick == 2 && decoded.tuning.attackRangeTiles == 3, "snapshot enemy codec should preserve integer tuning");
	Expect(Near(decoded.tuning.attackWindupSeconds, 0.60F) && Near(decoded.tuning.attackRecoverySeconds, 0.90F), "snapshot enemy codec should preserve timing tuning");
	Expect(decoded.combatStats.hitPoints == 10 && decoded.combatStats.attackPower == 11 && decoded.combatStats.defense == 12, "snapshot enemy codec should preserve combat stats");
	Expect(Near(decoded.stateTimerSeconds, 1.25F), "snapshot enemy codec should preserve state timer");
	Expect(reader.consumed(), "snapshot enemy codec should consume encoded enemy bytes");
}

void TestSnapshotEnemyCodecRejectsInvalidMoveState()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEntityCodec entityCodec;
	writer.writeU32(1);
	entityCodec.writeActorPosition(writer, dev::ActorPosition {});
	writer.writeU8(static_cast<uint8_t>(dev::EnemyMoveState::Recovering) + 1U);

	dev::SnapshotByteReader reader { bytes };
	dev::Enemy enemy;
	Expect(!dev::SnapshotEnemyCodec {}.readEnemy(reader, enemy), "snapshot enemy codec should reject invalid move state");
}

void TestSnapshotVectorCodecFramesCountedVectors()
{
	std::vector<int> values { 3, 4, 5 };
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotVectorCodec codec;
	codec.writeVector(writer, values, [](dev::SnapshotByteWriter &itemWriter, int value) {
		itemWriter.writeI32(value);
	});

	Expect(bytes.size() == 16, "snapshot vector codec should write count and item bytes");
	Expect(bytes.size() == 16 && bytes[0] == 3 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0, "snapshot vector codec should write count little-endian");

	std::vector<int> decoded;
	dev::SnapshotByteReader reader { bytes };
	Expect(codec.readVector(reader, decoded, [](dev::SnapshotByteReader &itemReader, int &value) {
		return itemReader.readI32(value);
	}), "snapshot vector codec should read encoded vectors");
	Expect(decoded == values, "snapshot vector codec should preserve vector items");
	Expect(reader.consumed(), "snapshot vector codec should consume encoded vector bytes");
}

void TestSnapshotVectorCodecRejectsTruncatedVectors()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	writer.writeU32(2);
	writer.writeI32(10);

	dev::SnapshotVectorCodec codec;
	dev::SnapshotByteReader reader { bytes };
	std::vector<int> decoded;
	Expect(!codec.readVector(reader, decoded, [](dev::SnapshotByteReader &itemReader, int &value) {
		return itemReader.readI32(value);
	}), "snapshot vector codec should reject truncated item data");
}

void TestSnapshotSchemaCodecRoundTripsOrderedSections()
{
	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 1, 2 });
	player.path.pushStep({ 2, 2 });
	player.inventory.capacity = 2;
	player.inventory.items.push_back({ .id = 101, .tile = { 3, 3 } });
	snapshot.players.push_back(player);
	dev::Enemy enemy = MakeEnemy({ 4, 4 });
	enemy.id = 102;
	enemy.tuning.attackRangeTiles = 2;
	snapshot.enemies.push_back(enemy);
	snapshot.items.push_back({ .id = 103, .tile = { 5, 5 }, .equipmentSlot = dev::EquipmentSlot::Armor, .combatModifiers = { .defense = 2 } });
	snapshot.combatants.push_back({
	    .target = { .type = dev::TargetType::Enemy, .id = 102, .tile = { 4, 4 } },
	    .stats = { .hitPoints = 6, .attackPower = 7, .defense = 8 },
	});
	snapshot.targets.push_back({ .type = dev::TargetType::Item, .id = 103, .tile = { 5, 5 } });

	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotSchemaCodec codec;
	codec.writeSnapshot(writer, snapshot);

	dev::SnapshotByteReader reader { bytes };
	dev::SimulationSnapshot decoded;
	Expect(codec.readSnapshot(reader, decoded), "snapshot schema codec should read encoded snapshot sections");
	Expect(decoded.players.size() == 1 && decoded.players[0].position.tile == dev::Point { 1, 2 }, "snapshot schema codec should preserve player section");
	Expect(decoded.players.size() == 1 && decoded.players[0].path.size() == 1, "snapshot schema codec should preserve player path inside section");
	Expect(decoded.enemies.size() == 1 && decoded.enemies[0].id == 102 && decoded.enemies[0].tuning.attackRangeTiles == 2, "snapshot schema codec should preserve enemy section");
	Expect(decoded.items.size() == 1 && decoded.items[0].id == 103 && decoded.items[0].combatModifiers.defense == 2, "snapshot schema codec should preserve item section");
	Expect(decoded.combatants.size() == 1 && decoded.combatants[0].stats.attackPower == 7, "snapshot schema codec should preserve combatant section");
	Expect(decoded.targets.size() == 1 && decoded.targets[0].type == dev::TargetType::Item, "snapshot schema codec should preserve target section");
	Expect(reader.consumed(), "snapshot schema codec should consume all schema bytes");
}

void TestSnapshotSchemaCodecRejectsIncompleteOrTrailingPayload()
{
	dev::SnapshotSchemaCodec codec;

	dev::SnapshotBytes incompleteBytes;
	dev::SnapshotByteWriter incompleteWriter { incompleteBytes };
	incompleteWriter.writeU32(0);
	dev::SnapshotByteReader incompleteReader { incompleteBytes };
	dev::SimulationSnapshot incomplete;
	Expect(!codec.readSnapshot(incompleteReader, incomplete), "snapshot schema codec should reject missing sections");

	dev::SimulationSnapshot emptySnapshot;
	dev::SnapshotBytes trailingBytes;
	dev::SnapshotByteWriter trailingWriter { trailingBytes };
	codec.writeSnapshot(trailingWriter, emptySnapshot);
	trailingWriter.writeU8(0xFFU);
	dev::SnapshotByteReader trailingReader { trailingBytes };
	dev::SimulationSnapshot trailing;
	Expect(!codec.readSnapshot(trailingReader, trailing), "snapshot schema codec should reject trailing payload bytes");
}

void TestSnapshotFrameCodecFramesPayloadBytes()
{
	dev::SnapshotBytes payload { 10, 20, 30 };
	dev::SnapshotFrameCodec frameCodec;
	dev::SnapshotBytes bytes = frameCodec.encode(payload);
	std::optional<dev::SnapshotBytes> decoded = frameCodec.decode(bytes);

	Expect(bytes.size() == 15, "snapshot frame codec should write header, payload, and checksum");
	Expect(bytes.size() == 15 && bytes[0] == 'I' && bytes[1] == 'G' && bytes[2] == 'G' && bytes[3] == 'Y', "snapshot frame codec should write magic");
	Expect(bytes.size() == 15 && bytes[4] == 7 && bytes[5] == 0 && bytes[6] == 0 && bytes[7] == 0, "snapshot frame codec should write version little-endian");
	Expect(decoded.has_value() && *decoded == payload, "snapshot frame codec should restore payload bytes");
}

void TestSnapshotFrameCodecRejectsInvalidFrames()
{
	dev::SnapshotBytes payload { 10, 20, 30 };
	dev::SnapshotFrameCodec frameCodec;
	dev::SnapshotBytes bytes = frameCodec.encode(payload);

	dev::SnapshotBytes badMagic = bytes;
	badMagic.resize(badMagic.size() - 4U);
	badMagic[0] = 'X';
	dev::SnapshotChecksum {}.appendTo(badMagic);
	Expect(!frameCodec.decode(badMagic).has_value(), "snapshot frame codec should reject bad magic");

	dev::SnapshotBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 8;
	dev::SnapshotChecksum {}.appendTo(badVersion);
	Expect(!frameCodec.decode(badVersion).has_value(), "snapshot frame codec should reject unsupported version");

	dev::SnapshotBytes truncated = bytes;
	truncated.pop_back();
	Expect(!frameCodec.decode(truncated).has_value(), "snapshot frame codec should reject truncated frames");
}

void TestSnapshotFileStoreSavesAndLoadsVersionedBytes()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_snapshot_store_test.bin";
	std::filesystem::remove(path);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 6, 7 });
	player.combatStats.hitPoints = 13;
	snapshot.players.push_back(player);

	dev::SnapshotFileStore store;
	Expect(store.save(path, snapshot), "snapshot file store should save snapshot bytes");
	std::optional<dev::SimulationSnapshot> loaded = store.load(path);
	Expect(loaded.has_value(), "snapshot file store should load saved bytes");
	Expect(loaded.has_value() && loaded->players.size() == 1 && loaded->players[0].position.tile == dev::Point { 6, 7 }, "loaded snapshot should preserve player position");
	Expect(loaded.has_value() && loaded->players[0].combatStats.hitPoints == 13, "loaded snapshot should preserve player hp");

	std::filesystem::remove(path);
}

void TestSnapshotFileStoreRejectsCorruptFile()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_snapshot_store_corrupt_test.bin";
	std::filesystem::remove(path);

	{
		std::ofstream output { path, std::ios::binary | std::ios::trunc };
		output << "not a snapshot";
	}

	dev::SnapshotFileStore store;
	Expect(!store.load(path).has_value(), "snapshot file store should reject corrupt files");

	std::filesystem::remove(path);
}

void TestSaveGameServiceSavesAndLoadsWorld()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_save_game_service_test.bin";
	std::filesystem::remove(path);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SimulationWorld world;
	dev::CombatEventRecorder combatEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 8, 2 }));
	world.players[0].combatStats.hitPoints = 17;
	world.players[0].inventory.items.push_back({ .id = 73, .tile = { 0, 0 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 70, .tile = { 9, 2 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 6, .attackPower = 4, .defense = 1 },
	});
	world.items.push_back({ .id = 72, .tile = { 11, 2 } });
	world.targets.add(target);
	world.targets.add({ .type = dev::TargetType::Object, .id = 71, .tile = { 10, 2 } });
	world.targets.add({ .type = dev::TargetType::Item, .id = 72, .tile = { 11, 2 } });

	dev::SaveGameService saves;
	Expect(saves.saveWorld(path, world), "save game service should save world");

	world.players[0].position.tile = { 0, 0 };
	world.players[0].combatStats.hitPoints = 1;
	world.players[0].inventory.items.clear();
	world.items.clear();
	world.combat.registry().replaceAll({});
	world.targets.clear();
	Expect(saves.loadWorld(path, world), "save game service should load world");

	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target restoredEnemy = world.targets.resolveAtTile({ 9, 2 });
	dev::Target restoredObject = world.targets.resolveAtTile({ 10, 2 });
	dev::Target restoredItem = world.targets.resolveAtTile({ 11, 2 });
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 8, 2 }, "save game service should restore player position");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 17, "save game service should restore player hp");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 73, "save game service should restore player inventory");
	Expect(world.items.size() == 1 && world.items[0].id == 72, "save game service should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 6, "save game service should restore combat state");
	Expect(restoredEnemy.type == dev::TargetType::Enemy && restoredEnemy.id == 70, "save game service should restore enemy target");
	Expect(restoredObject.type == dev::TargetType::Object && restoredObject.id == 71, "save game service should restore object target");
	Expect(restoredItem.type == dev::TargetType::Item && restoredItem.id == 72, "save game service should restore item target");
	Expect(world.combatEvents == &combatEvents, "save game service should preserve world event sinks");

	std::filesystem::remove(path);
}

void TestSaveSlotServiceListsMetadata()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_save_slot_metadata_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 3, 9 }));
	world.players[0].combatStats.hitPoints = 14;
	world.enemies.push_back(MakeEnemy({ 4, 9 }));
	world.items.push_back({ .id = 90, .tile = { 5, 9 } });
	world.targets.add({ .type = dev::TargetType::Item, .id = 90, .tile = { 5, 9 } });

	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(1, world), "save slot service should save occupied slot");

	{
		std::ofstream corrupt { slots.pathForSlot(2), std::ios::binary | std::ios::trunc };
		corrupt << "corrupt";
	}

	std::vector<dev::SaveSlotMetadata> listed = slots.listSlots(1, 3);
	Expect(listed.size() == 3, "save slot service should list requested slot count");
	Expect(listed.size() == 3 && listed[0].slotId == 1 && listed[0].occupied && listed[0].valid, "saved slot should be occupied and valid");
	Expect(listed.size() == 3 && listed[0].playerTile == dev::Point { 3, 9 }, "slot metadata should include player tile");
	Expect(listed.size() == 3 && listed[0].playerHitPoints == 14, "slot metadata should include player hp");
	Expect(listed.size() == 3 && listed[0].enemyCount == 1, "slot metadata should include enemy count");
	Expect(listed.size() == 3 && listed[1].slotId == 2 && listed[1].occupied && !listed[1].valid, "corrupt slot should be occupied but invalid");
	Expect(listed.size() == 3 && listed[2].slotId == 3 && !listed[2].occupied && !listed[2].valid, "missing slot should be empty and invalid");

	std::filesystem::remove_all(root);
}

void TestSaveSlotServiceLoadsWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_save_slot_load_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld source;
	source.players.push_back(MakePlayer({ 7, 1 }));
	source.players[0].combatStats.hitPoints = 19;
	source.players[0].inventory.items.push_back({ .id = 82, .tile = { 0, 0 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 80, .tile = { 8, 1 } };
	source.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 8, .attackPower = 5, .defense = 1 },
	});
	source.items.push_back({ .id = 81, .tile = { 9, 1 } });
	source.targets.add(target);
	source.targets.add({ .type = dev::TargetType::Item, .id = 81, .tile = { 9, 1 } });

	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(4, source), "save slot service should save loadable slot");

	dev::SimulationWorld loaded;
	dev::CombatEventRecorder combatEvents;
	loaded.setCombatEventSink(&combatEvents);
	Expect(slots.loadSlot(4, loaded), "save slot service should load saved slot");

	const dev::Combatant *combatant = loaded.combat.registry().find(target);
	dev::Target loadedTarget = loaded.targets.resolveAtTile({ 8, 1 });
	dev::Target loadedItemTarget = loaded.targets.resolveAtTile({ 9, 1 });
	Expect(loaded.players.size() == 1 && loaded.players[0].position.tile == dev::Point { 7, 1 }, "slot load should restore player tile");
	Expect(loaded.players.size() == 1 && loaded.players[0].combatStats.hitPoints == 19, "slot load should restore player hp");
	Expect(loaded.players.size() == 1 && loaded.players[0].inventory.items.size() == 1 && loaded.players[0].inventory.items[0].id == 82, "slot load should restore player inventory");
	Expect(loaded.items.size() == 1 && loaded.items[0].id == 81, "slot load should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 8, "slot load should restore combat state");
	Expect(loadedTarget.type == dev::TargetType::Enemy && loadedTarget.id == 80, "slot load should restore target registry");
	Expect(loadedItemTarget.type == dev::TargetType::Item && loadedItemTarget.id == 81, "slot load should restore item target");
	Expect(loaded.combatEvents == &combatEvents, "slot load should preserve world event sinks");

	std::filesystem::remove_all(root);
}

void TestGameSessionStartsNewGameAndUpdates()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_new_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 16 });
	session.world().commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationFrameEvents frame = session.update(0.016F);

	Expect(session.mode() == dev::GameSessionMode::Gameplay, "new game should enter gameplay mode");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 16, "new game should apply player settings");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 1, 0 }, "game session update should advance world");
	Expect(!frame.movementEvents().empty(), "game session update should return frame events");

	std::filesystem::remove_all(root);
}

void TestNewGameWorldBuilderCreatesPlayerWorld()
{
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;

	dev::SimulationWorld world = dev::NewGameWorldBuilder {}.build(
	    { .playerStart = { 7, 3 }, .playerHitPoints = 24 },
	    &movementEvents,
	    &combatEvents);

	Expect(world.players.size() == 1, "new game world builder should create one starting player");
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 7, 3 }, "new game world builder should set player tile");
	Expect(world.players.size() == 1 && world.players[0].position.future == dev::Point { 7, 3 }, "new game world builder should initialize future tile");
	Expect(world.players.size() == 1 && world.players[0].position.previous == dev::Point { 7, 3 }, "new game world builder should initialize previous tile");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 24, "new game world builder should apply starting hit points");
	Expect(world.movementEvents == &movementEvents, "new game world builder should preserve movement sink");
	Expect(world.combatEvents == &combatEvents, "new game world builder should preserve combat sink");
}

void TestSessionWorldSlotLoaderLoadsWorldPreservingSinks()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_world_slot_loader_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld saved;
	saved.players.push_back(MakePlayer({ 8, 8 }));
	saved.players[0].combatStats.hitPoints = 13;
	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(1, saved), "session world slot loader setup should save world");

	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	dev::SimulationWorld current;
	current.movementEvents = &movementEvents;
	current.setCombatEventSink(&combatEvents);
	current.players.push_back(MakePlayer({ 1, 1 }));

	Expect(dev::SessionWorldSlotLoader {}.load(slots, 1, current), "session world slot loader should load saved world");
	Expect(current.players.size() == 1 && current.players[0].position.tile == dev::Point { 8, 8 }, "session world slot loader should replace world on success");
	Expect(current.players.size() == 1 && current.players[0].combatStats.hitPoints == 13, "session world slot loader should restore saved player state");
	Expect(current.movementEvents == &movementEvents && current.combatEvents == &combatEvents, "session world slot loader should preserve event sinks");

	current.players[0].position.tile = { 2, 2 };
	Expect(!dev::SessionWorldSlotLoader {}.load(slots, 99, current), "session world slot loader should reject missing slots");
	Expect(current.players.size() == 1 && current.players[0].position.tile == dev::Point { 2, 2 }, "session world slot loader should preserve current world on failed load");
	Expect(current.movementEvents == &movementEvents && current.combatEvents == &combatEvents, "session world slot loader should preserve sinks on failed load");

	std::filesystem::remove_all(root);
}

void TestSessionWorldSlotSaverRequiresActiveSession()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_world_slot_saver_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 6, 6 }));
	world.players[0].combatStats.hitPoints = 11;
	dev::SaveSlotService slots { root };

	dev::SessionWorldSlotSaver saver;
	Expect(!saver.save(slots, 1, world, dev::GameSessionMode::Empty), "session world slot saver should reject empty sessions");
	Expect(!slots.metadataForSlot(1).occupied, "rejected session save should not create a slot file");
	Expect(saver.save(slots, 2, world, dev::GameSessionMode::Inventory), "session world slot saver should save active inventory sessions");

	dev::SimulationWorld loaded;
	Expect(slots.loadSlot(2, loaded), "session world slot saver should write loadable slot data");
	Expect(loaded.players.size() == 1 && loaded.players[0].position.tile == dev::Point { 6, 6 }, "session world slot saver should save player position");
	Expect(loaded.players.size() == 1 && loaded.players[0].combatStats.hitPoints == 11, "session world slot saver should save player hp");

	std::filesystem::remove_all(root);
}

void TestSessionFrameUpdaterAppliesModePolicy()
{
	dev::SimulationWorld world;
	dev::SimulationClock clock;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SessionFrameUpdater updater;
	dev::SimulationFrameEvents emptyFrame = updater.update(world, clock, dev::GameSessionMode::Empty, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "session frame updater should not run empty sessions");
	Expect(emptyFrame.movementEvents().empty(), "session frame updater should not emit empty-session frame events");

	dev::SimulationFrameEvents inventoryFrame = updater.update(world, clock, dev::GameSessionMode::Inventory, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "session frame updater should freeze inventory sessions");
	Expect(inventoryFrame.movementEvents().empty(), "session frame updater should not drain movement in inventory mode");

	dev::SimulationFrameEvents gameplayFrame = updater.update(world, clock, dev::GameSessionMode::Gameplay, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "session frame updater should advance gameplay sessions");
	Expect(!gameplayFrame.movementEvents().empty(), "session frame updater should return gameplay frame events");
}

void TestGameSessionPausedModePreservesCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_pause_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	session.startNewGame({ .playerStart = { 0, 0 } });
	session.world().commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	session.setMode(dev::GameSessionMode::Paused);
	(void)session.update(0.016F);
	Expect(session.world().players[0].position.tile == dev::Point { 0, 0 }, "paused session should not drain or move command");

	session.setMode(dev::GameSessionMode::Gameplay);
	(void)session.update(0.016F);
	Expect(session.world().players[0].position.tile == dev::Point { 1, 0 }, "gameplay session should resume preserved command");

	std::filesystem::remove_all(root);
}

void TestSessionModePolicyMapsModesToFramePolicy()
{
	dev::SessionModePolicy policy;

	dev::SimulationFramePolicy gameplay = policy.framePolicyFor(dev::GameSessionMode::Gameplay);
	dev::SimulationFramePolicy paused = policy.framePolicyFor(dev::GameSessionMode::Paused);
	dev::SimulationFramePolicy inventory = policy.framePolicyFor(dev::GameSessionMode::Inventory);
	dev::SimulationFramePolicy empty = policy.framePolicyFor(dev::GameSessionMode::Empty);

	Expect(gameplay.acceptCommands && gameplay.updatePlayers && gameplay.updateEnemies, "session mode policy should let gameplay update actors");
	Expect(!paused.acceptCommands && !paused.updatePlayers && !paused.updateEnemies, "session mode policy should freeze paused sessions");
	Expect(!inventory.acceptCommands && !inventory.updatePlayers && !inventory.updateEnemies, "session mode policy should freeze inventory sessions");
	Expect(!empty.acceptCommands && !empty.updatePlayers && !empty.updateEnemies, "session mode policy should freeze empty sessions");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Gameplay) == dev::SimulationMode::Gameplay, "session mode policy should expose gameplay simulation mode");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Paused) == dev::SimulationMode::Paused, "session mode policy should expose paused simulation mode");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Inventory) == dev::SimulationMode::Inventory, "session mode policy should expose inventory simulation mode");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Empty) == dev::SimulationMode::Paused, "session mode policy should describe empty sessions with paused frame policy");
}

void TestSessionModePolicyGuardsTransitions()
{
	dev::SessionModePolicy policy;

	Expect(policy.hasActiveWorld(dev::GameSessionMode::Gameplay), "session mode policy should treat gameplay as active");
	Expect(!policy.hasActiveWorld(dev::GameSessionMode::Empty), "session mode policy should treat empty as inactive");
	Expect(!policy.canTransition(dev::GameSessionMode::Empty, dev::GameSessionMode::Gameplay), "session mode policy should reject activating an empty session through mode change");
	Expect(policy.canTransition(dev::GameSessionMode::Gameplay, dev::GameSessionMode::Inventory), "session mode policy should allow active session mode changes");
	Expect(policy.canTransition(dev::GameSessionMode::Inventory, dev::GameSessionMode::Empty), "session mode policy should allow returning to empty mode");
}

void TestSessionModeChangerAppliesAllowedTransitionsOnly()
{
	dev::SessionModeChanger changer;
	dev::GameSessionMode empty = dev::GameSessionMode::Empty;
	dev::GameSessionMode active = dev::GameSessionMode::Gameplay;

	Expect(!changer.change(empty, dev::GameSessionMode::Inventory), "session mode changer should reject activating empty sessions");
	Expect(empty == dev::GameSessionMode::Empty, "session mode changer should preserve mode after rejected transition");
	Expect(changer.change(active, dev::GameSessionMode::Inventory), "session mode changer should allow active session mode changes");
	Expect(active == dev::GameSessionMode::Inventory, "session mode changer should apply allowed active transition");
	Expect(changer.change(active, dev::GameSessionMode::Empty), "session mode changer should allow returning to empty mode");
	Expect(active == dev::GameSessionMode::Empty, "session mode changer should apply empty transition");
}

void TestGameSessionSaveLoadPreservesSinksAndResetsClock()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_save_load_test";
	std::filesystem::remove_all(root);

	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	dev::GameSession session { root };
	session.world().movementEvents = &movementEvents;
	session.world().setCombatEventSink(&combatEvents);
	session.startNewGame({ .playerStart = { 4, 4 }, .playerHitPoints = 18 });
	session.clock().setTimeScale(0.25F);
	session.clock().triggerHitStop(1.0F);
	Expect(session.saveToSlot(1), "game session should save active world");

	session.world().players[0].position.tile = { 9, 9 };
	session.world().players[0].combatStats.hitPoints = 1;
	Expect(session.loadFromSlot(1), "game session should load saved world");

	Expect(session.world().players[0].position.tile == dev::Point { 4, 4 }, "game session load should restore player position");
	Expect(session.world().players[0].combatStats.hitPoints == 18, "game session load should restore player hp");
	Expect(session.world().movementEvents == &movementEvents && session.world().combatEvents == &combatEvents, "game session load should preserve event sinks");
	Expect(session.clock().timeScale() == 1.0F && session.clock().hitStopRemainingSeconds() == 0.0F, "game session load should reset transient clock state");

	std::filesystem::remove_all(root);
}

void TestGameSessionMissingLoadKeepsCurrentWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_missing_load_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	session.startNewGame({ .playerStart = { 5, 5 }, .playerHitPoints = 12 });

	Expect(!session.loadFromSlot(99), "game session should reject missing slot load");
	Expect(session.hasActiveWorld(), "failed load should keep current active world");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 5, 5 }, "failed load should preserve player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 12, "failed load should preserve player hp");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherAppliesLifecycleCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_dispatch_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };

	dev::SessionCommandResult start = dispatcher.dispatch({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 2, 6 }, .playerHitPoints = 15 },
	});
	Expect(start.type == dev::SessionCommandResultType::Applied, "session command should start new game");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 2, 6 }, "new game command should apply settings");

	dev::SessionCommandResult save = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(save.type == dev::SessionCommandResultType::Applied, "session command should save active slot");

	session.world().players[0].position.tile = { 9, 9 };
	dev::SessionCommandResult load = dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});
	Expect(load.type == dev::SessionCommandResultType::Applied, "session command should load existing slot");
	Expect(session.world().players[0].position.tile == dev::Point { 2, 6 }, "load command should restore saved world");

	dev::SessionCommandResult pause = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	Expect(pause.type == dev::SessionCommandResultType::Applied, "session command should apply mode change");
	Expect(session.mode() == dev::GameSessionMode::Paused, "mode command should change session mode");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherRejectsInvalidLifecycleCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_reject_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };

	dev::SessionCommandResult saveEmpty = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(saveEmpty.type == dev::SessionCommandResultType::Rejected, "session command should reject saving empty session");

	dev::SessionCommandResult missingLoad = dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	Expect(missingLoad.type == dev::SessionCommandResultType::Rejected, "session command should reject missing load slot");
	Expect(!session.hasActiveWorld(), "rejected missing load should leave empty session empty");

	dev::SessionCommandResult missingMode = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	});
	Expect(missingMode.type == dev::SessionCommandResultType::Rejected, "session command should reject missing mode payload");

	std::filesystem::remove_all(root);
}

void TestSessionCommandApplierMapsLifecycleOutcomes()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_applier_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandApplier applier { session };

	dev::SessionCommandApplication start = applier.apply({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 3, 7 } },
	});
	Expect(start.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply start command");
	Expect(start.eventType == dev::SessionEventType::GameStarted, "session command applier should map start to GameStarted");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 3, 7 }, "session command applier should mutate session for start command");

	dev::SessionCommandApplication save = applier.apply({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(save.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply save command");
	Expect(save.eventType == dev::SessionEventType::SaveCompleted, "session command applier should map successful save to SaveCompleted");

	session.world().players[0].position.tile = { 9, 9 };
	dev::SessionCommandApplication load = applier.apply({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});
	Expect(load.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply load command");
	Expect(load.eventType == dev::SessionEventType::LoadCompleted, "session command applier should map successful load to LoadCompleted");
	Expect(session.world().players[0].position.tile == dev::Point { 3, 7 }, "session command applier should restore saved world");

	dev::SessionCommandApplication mode = applier.apply({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});
	Expect(mode.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply mode command");
	Expect(mode.eventType == dev::SessionEventType::ModeChanged, "session command applier should map successful mode change to ModeChanged");

	std::filesystem::remove_all(root);
}

void TestSessionCommandApplierMapsRejectedOutcomes()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_applier_reject_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandApplier applier { session };

	dev::SessionCommandApplication save = applier.apply({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(save.result.type == dev::SessionCommandResultType::Rejected, "session command applier should reject saving empty sessions");
	Expect(save.eventType == dev::SessionEventType::SaveFailed, "session command applier should map rejected save to SaveFailed");

	dev::SessionCommandApplication load = applier.apply({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	Expect(load.result.type == dev::SessionCommandResultType::Rejected, "session command applier should reject missing load slots");
	Expect(load.eventType == dev::SessionEventType::LoadFailed, "session command applier should map rejected load to LoadFailed");

	dev::SessionCommandApplication mode = applier.apply({
	    .type = dev::SessionCommandType::SetMode,
	});
	Expect(mode.result.type == dev::SessionCommandResultType::Rejected, "session command applier should reject mode commands without payload");
	Expect(mode.eventType == dev::SessionEventType::ModeChangeRejected, "session command applier should map rejected mode to ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionEventEmitterBuildsLifecycleEvents()
{
	dev::SessionEventRecorder events;
	dev::SessionCommand command {
		.type = dev::SessionCommandType::LoadSlot,
		.slotId = 4,
		.mode = dev::GameSessionMode::Inventory,
	};

	dev::SessionEventEmitter { &events }.emit(command, dev::SessionEventType::LoadCompleted);
	dev::SessionEventEmitter {}.emit(command, dev::SessionEventType::LoadFailed);

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 1, "session event emitter should ignore missing event sinks");
	Expect(recorded.size() == 1 && recorded[0].type == dev::SessionEventType::LoadCompleted, "session event emitter should preserve event type");
	Expect(recorded.size() == 1 && recorded[0].commandType == dev::SessionCommandType::LoadSlot, "session event emitter should preserve command type");
	Expect(recorded.size() == 1 && recorded[0].slotId == std::optional<dev::SaveSlotId> { 4 }, "session event emitter should preserve slot id");
	Expect(recorded.size() == 1 && recorded[0].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "session event emitter should preserve mode payload");
}

void TestSessionCommandDispatcherEmitsSuccessEvents()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_event_success_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 1 } },
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 2,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 2,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 4, "session dispatcher should emit success lifecycle events");
	Expect(recorded.size() == 4 && recorded[0].type == dev::SessionEventType::GameStarted, "start command should emit GameStarted");
	Expect(recorded.size() == 4 && recorded[1].type == dev::SessionEventType::SaveCompleted, "save command should emit SaveCompleted");
	Expect(recorded.size() == 4 && recorded[1].slotId == std::optional<dev::SaveSlotId> { 2 }, "save event should include slot id");
	Expect(recorded.size() == 4 && recorded[2].type == dev::SessionEventType::LoadCompleted, "load command should emit LoadCompleted");
	Expect(recorded.size() == 4 && recorded[3].type == dev::SessionEventType::ModeChanged, "mode command should emit ModeChanged");
	Expect(recorded.size() == 4 && recorded[3].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "mode event should include target mode");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherEmitsFailureEvents()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_event_failure_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 3,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	});

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 3, "session dispatcher should emit failure lifecycle events");
	Expect(recorded.size() == 3 && recorded[0].type == dev::SessionEventType::SaveFailed, "empty save command should emit SaveFailed");
	Expect(recorded.size() == 3 && recorded[0].slotId == std::optional<dev::SaveSlotId> { 3 }, "save failure should include slot id");
	Expect(recorded.size() == 3 && recorded[1].type == dev::SessionEventType::LoadFailed, "missing load command should emit LoadFailed");
	Expect(recorded.size() == 3 && recorded[1].slotId == std::optional<dev::SaveSlotId> { 99 }, "load failure should include slot id");
	Expect(recorded.size() == 3 && recorded[2].type == dev::SessionEventType::ModeChangeRejected, "missing mode command should emit ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionCommandReplayAppliesLifecycleSequence()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_replay_sequence_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 3, 4 }, .playerHitPoints = 12 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = replayer.replay(log);

	Expect(!log.empty(), "session command log should record commands");
	Expect(results.size() == 4, "session replay should return one result per command");
	Expect(results.size() == 4 && results[0].type == dev::SessionCommandResultType::Applied, "replay should apply start command");
	Expect(results.size() == 4 && results[1].type == dev::SessionCommandResultType::Applied, "replay should apply save command");
	Expect(results.size() == 4 && results[2].type == dev::SessionCommandResultType::Applied, "replay should apply mode command");
	Expect(results.size() == 4 && results[3].type == dev::SessionCommandResultType::Applied, "replay should apply load command");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 3, 4 }, "session replay should restore final player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 12, "session replay should restore final player hp");
	Expect(session.mode() == dev::GameSessionMode::Gameplay, "load command in replay should return session to gameplay");
	Expect(events.events().size() == 4, "session replay should emit lifecycle events");
	Expect(events.events().size() == 4 && events.events()[0].type == dev::SessionEventType::GameStarted, "session replay should emit start event");
	Expect(events.events().size() == 4 && events.events()[3].type == dev::SessionEventType::LoadCompleted, "session replay should emit load event");

	std::filesystem::remove_all(root);
}

void TestSessionCommandReplayReportsRejectedCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_replay_reject_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	});

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = replayer.replay(log);

	Expect(results.size() == 2, "session replay should report rejected command results");
	Expect(results.size() == 2 && results[0].type == dev::SessionCommandResultType::Rejected, "session replay should reject missing load");
	Expect(results.size() == 2 && results[1].type == dev::SessionCommandResultType::Rejected, "session replay should reject malformed mode command");
	Expect(!session.hasActiveWorld(), "replayed rejected lifecycle commands should not create active world");
	Expect(events.events().size() == 2, "replayed rejected lifecycle commands should emit failure events");
	Expect(events.events().size() == 2 && events.events()[0].type == dev::SessionEventType::LoadFailed, "replayed missing load should emit LoadFailed");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::SessionEventType::ModeChangeRejected, "replayed malformed mode should emit ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionCommandCodecRoundTripsCommands()
{
	dev::SessionCommandCodec codec;
	std::vector<dev::SessionCommand> commands {
		{
		    .type = dev::SessionCommandType::StartNewGame,
		    .newGameSettings = dev::NewGameSettings { .playerStart = { 6, 8 }, .playerHitPoints = 17 },
		},
		{
		    .type = dev::SessionCommandType::SaveSlot,
		    .slotId = 3,
		},
		{
		    .type = dev::SessionCommandType::LoadSlot,
		    .slotId = 4,
		},
		{
		    .type = dev::SessionCommandType::SetMode,
		    .mode = dev::GameSessionMode::Inventory,
		},
	};

	for (const dev::SessionCommand &command : commands) {
		dev::SessionCommandPacket packet = codec.toPacket(command);
		dev::SessionCommandBytes bytes = codec.encode(packet);
		std::optional<dev::SessionCommandPacket> decodedPacket = codec.decode(bytes);
		Expect(decodedPacket.has_value(), "session command packet should decode");
		std::optional<dev::SessionCommand> decoded = decodedPacket.has_value()
		    ? codec.fromPacket(*decodedPacket)
		    : std::nullopt;
		Expect(decoded.has_value(), "session command packet should become command");
		if (!decoded.has_value())
			continue;
		Expect(decoded->type == command.type, "session command codec should preserve command type");
		Expect(decoded->newGameSettings.has_value() == command.newGameSettings.has_value(), "session command codec should preserve new-game payload presence");
		Expect(decoded->slotId == command.slotId, "session command codec should preserve slot id");
		Expect(decoded->mode == command.mode, "session command codec should preserve mode");
		if (command.newGameSettings.has_value()) {
			Expect(decoded->newGameSettings->playerStart == command.newGameSettings->playerStart, "session command codec should preserve player start");
			Expect(decoded->newGameSettings->playerHitPoints == command.newGameSettings->playerHitPoints, "session command codec should preserve player hp");
		}
	}
}

void TestSessionCommandCodecRejectsInvalidPackets()
{
	dev::SessionCommandCodec codec;

	dev::SessionCommandPacket invalidType {
		.commandType = 99,
	};
	Expect(!codec.fromPacket(invalidType).has_value(), "session command codec should reject invalid command type");

	dev::SessionCommandPacket saveWithoutSlot {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
	};
	Expect(!codec.fromPacket(saveWithoutSlot).has_value(), "session command codec should reject save command without slot");

	dev::SessionCommandPacket modeWithoutPayload {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
	};
	Expect(!codec.fromPacket(modeWithoutPayload).has_value(), "session command codec should reject mode command without mode payload");

	dev::SessionCommandPacket invalidMode {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		.hasMode = 1,
		.mode = 99,
	};
	Expect(!codec.fromPacket(invalidMode).has_value(), "session command codec should reject invalid mode");

	dev::SessionCommandPacket extraPayload {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::LoadSlot),
		.hasNewGameSettings = 1,
		.hasSlotId = 1,
		.slotId = 5,
	};
	Expect(!codec.fromPacket(extraPayload).has_value(), "session command codec should reject unexpected payload fields");

	dev::SessionCommandBytes shortBytes = codec.encode(codec.toPacket({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	}));
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "session command codec should reject wrong byte size");
}

void TestSessionCommandPacketValidatorRejectsMalformedPayloads()
{
	dev::SessionCommandPacketValidator validator;

	Expect(validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::StartNewGame),
	           .hasNewGameSettings = 1,
	           .playerStartX = 2,
	           .playerStartY = 3,
	           .playerHitPoints = 14,
	       }),
	    "session command packet validator should accept valid new-game packets");
	Expect(validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
	           .hasSlotId = 1,
	           .slotId = 7,
	       }),
	    "session command packet validator should accept valid slot packets");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
	           .hasSlotId = 2,
	           .slotId = 7,
	       }),
	    "session command packet validator should reject non-boolean payload flags");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
	           .hasMode = 1,
	           .mode = 99,
	       }),
	    "session command packet validator should reject invalid modes");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::LoadSlot),
	           .hasNewGameSettings = 1,
	           .hasSlotId = 1,
	           .slotId = 3,
	       }),
	    "session command packet validator should reject unexpected payload fields");
}

void TestSessionCommandByteStreamWritesLittleEndianPrimitives()
{
	std::vector<uint8_t> bytes;
	dev::SessionCommandByteWriter writer { bytes };
	writer.writeU8(0xABU);
	writer.writeU16(0x1234U);
	writer.writeU32(0xABCDEF12U);

	Expect(bytes.size() == 7, "session command byte writer should append primitive bytes");
	Expect(bytes.size() == 7 && bytes[1] == 0x34U && bytes[2] == 0x12U, "session command byte writer should write u16 little-endian");
	Expect(bytes.size() == 7 && bytes[3] == 0x12U && bytes[4] == 0xEFU && bytes[5] == 0xCDU && bytes[6] == 0xABU, "session command byte writer should write u32 little-endian");

	dev::SessionCommandByteReader reader { bytes };
	uint8_t byte = 0;
	uint16_t shortValue = 0;
	uint32_t wordValue = 0;
	Expect(reader.readU8(byte) && byte == 0xABU, "session command byte reader should read u8");
	Expect(reader.readU16(shortValue) && shortValue == 0x1234U, "session command byte reader should read u16");
	Expect(reader.readU32(wordValue) && wordValue == 0xABCDEF12U, "session command byte reader should read u32");
	Expect(reader.consumed(), "session command byte reader should report consumed bytes");

	dev::SessionCommandByteReader offsetReader { bytes, 3 };
	Expect(offsetReader.readU32(wordValue) && wordValue == 0xABCDEF12U, "session command byte reader should read from a starting offset");
}

void TestSessionCommandByteStreamRejectsShortReads()
{
	std::vector<uint8_t> bytes { 1, 2, 3 };
	dev::SessionCommandByteReader reader { bytes };
	uint32_t wordValue = 0;
	uint8_t first = 0;

	Expect(!reader.readU32(wordValue), "session command byte reader should reject short u32 reads");
	Expect(reader.offset() == 0, "session command byte reader should not advance after failed u32 reads");
	Expect(reader.readU8(first) && first == 1, "session command byte reader should continue after failed reads");
}

void TestSessionCommandPacketByteCodecRoundTripsPackets()
{
	dev::SessionCommandPacketByteCodec codec;
	dev::SessionCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		.hasSlotId = 1,
		.slotId = 0x01020304U,
	};

	dev::SessionCommandBytes bytes = codec.encode(packet);
	std::optional<dev::SessionCommandPacket> decoded = codec.decode(bytes);

	Expect(bytes.size() == 25, "session command packet byte codec should write fixed packet size");
	Expect(bytes.size() == 25 && bytes[9] == 0x04 && bytes[10] == 0x03 && bytes[11] == 0x02 && bytes[12] == 0x01, "session command packet byte codec should write slot id little-endian");
	Expect(decoded.has_value(), "session command packet byte codec should decode valid bytes");
	Expect(decoded.has_value() && decoded->commandType == packet.commandType, "session command packet byte codec should preserve command type");
	Expect(decoded.has_value() && decoded->hasSlotId == 1 && decoded->slotId == packet.slotId, "session command packet byte codec should preserve slot payload");
}

void TestSessionCommandPacketByteCodecRejectsInvalidBytes()
{
	dev::SessionCommandPacketByteCodec codec;
	dev::SessionCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		.hasMode = 1,
		.mode = static_cast<uint8_t>(dev::GameSessionMode::Inventory),
	};

	dev::SessionCommandBytes shortBytes = codec.encode(packet);
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "session command packet byte codec should reject wrong byte size");

	dev::SessionCommandBytes invalidPacketBytes = codec.encode(packet);
	invalidPacketBytes[14] = 99;
	Expect(!codec.decode(invalidPacketBytes).has_value(), "session command packet byte codec should reject invalid decoded packets");
}

void TestSessionCommandLogCodecRoundTripsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_log_codec_replay_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 10, 4 }, .playerHitPoints = 14 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 5,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 5,
	});

	dev::SessionCommandLogCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(log);
	std::optional<dev::SessionCommandLog> decoded = codec.decode(bytes);
	Expect(decoded.has_value(), "session command log codec should decode its own bytes");
	Expect(decoded.has_value() && decoded->commands().size() == 3, "session command log codec should preserve command count");
	Expect(decoded.has_value() && decoded->commands()[0].newGameSettings->playerStart == dev::Point { 10, 4 }, "session command log codec should preserve new-game settings");
	Expect(decoded.has_value() && decoded->commands()[1].slotId == std::optional<dev::SaveSlotId> { 5 }, "session command log codec should preserve save slot");

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = decoded.has_value()
	    ? replayer.replay(*decoded)
	    : std::vector<dev::SessionCommandResult> {};

	Expect(results.size() == 3, "decoded session command log should replay");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 10, 4 }, "decoded session command log should reproduce session state");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 14, "decoded session command log should reproduce player hp");

	std::filesystem::remove_all(root);
}

void TestSessionCommandLogCodecRejectsInvalidBytes()
{
	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 1 } },
	});

	dev::SessionCommandLogCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(log);

	dev::SessionCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "session command log codec should reject bad magic");

	dev::SessionCommandLogBytes badVersion = bytes;
	badVersion[4] = 2;
	Expect(!codec.decode(badVersion).has_value(), "session command log codec should reject bad version");

	dev::SessionCommandLogBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "session command log codec should reject truncated bytes");

	dev::SessionCommandLogBytes corrupted = bytes;
	corrupted[12] ^= 0x01U;
	Expect(!codec.decode(corrupted).has_value(), "session command log codec should reject checksum mismatch");
}

void TestSessionCommandLogChecksumValidatesTrailingChecksum()
{
	dev::SessionCommandLogBytes bytes { 1, 2, 3, 4 };
	dev::SessionCommandLogChecksum checksum;
	const uint32_t expected = checksum.compute(bytes, bytes.size());

	checksum.appendTo(bytes);

	Expect(bytes.size() == 8, "session command log checksum should append four checksum bytes");
	Expect(checksum.hasValidTrailingChecksum(bytes, 4), "session command log checksum should validate appended checksum");
	Expect(expected == checksum.compute(bytes, 4), "session command log checksum should compute payload hash only");

	bytes[0] ^= 0xFFU;
	Expect(!checksum.hasValidTrailingChecksum(bytes, 4), "session command log checksum should reject mutated payload");
}

void TestSessionCommandPacketListCodecFramesPacketBytes()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		    .hasMode = 1,
		    .mode = static_cast<uint8_t>(dev::GameSessionMode::Inventory),
		}),
	};

	dev::SessionCommandPacketListCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(packets);
	std::optional<std::vector<dev::SessionCommandBytes>> decoded = codec.decode(bytes);

	Expect(bytes.size() == 54, "session command packet list codec should write count and packet bytes");
	Expect(bytes.size() == 54 && bytes[0] == 2 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0, "session command packet list codec should write count little-endian");
	Expect(decoded.has_value() && *decoded == packets, "session command packet list codec should restore packet bytes");
}

void TestSessionCommandPacketListCodecRejectsInvalidSizes()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
	};

	dev::SessionCommandPacketListCodec codec;
	dev::SessionCommandLogBytes truncated = codec.encode(packets);
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "session command packet list codec should reject truncated packet lists");

	dev::SessionCommandLogBytes wrongCount = codec.encode(packets);
	wrongCount[0] = 2;
	Expect(!codec.decode(wrongCount).has_value(), "session command packet list codec should reject mismatched packet counts");
}

void TestSessionCommandLogFrameCodecFramesPacketBytes()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		    .hasMode = 1,
		    .mode = static_cast<uint8_t>(dev::GameSessionMode::Inventory),
		}),
	};

	dev::SessionCommandLogFrameCodec frameCodec;
	dev::SessionCommandLogBytes bytes = frameCodec.encode(packets);
	std::optional<std::vector<dev::SessionCommandBytes>> decoded = frameCodec.decode(bytes);

	Expect(bytes.size() == 66, "session command log frame codec should write header, packets, and checksum");
	Expect(bytes.size() == 66 && bytes[0] == 'I' && bytes[1] == 'S' && bytes[2] == 'C' && bytes[3] == 'L', "session command log frame codec should write magic");
	Expect(bytes.size() == 66 && bytes[4] == 1 && bytes[8] == 2, "session command log frame codec should write version and packet list count");
	Expect(decoded.has_value() && decoded->size() == 2, "session command log frame codec should restore packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packets[0], "session command log frame codec should preserve first packet");
	Expect(decoded.has_value() && (*decoded)[1] == packets[1], "session command log frame codec should preserve second packet");
}

void TestSessionCommandLogFrameCodecRejectsInvalidFrames()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
	};
	dev::SessionCommandLogFrameCodec frameCodec;
	dev::SessionCommandLogBytes bytes = frameCodec.encode(packets);

	dev::SessionCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!frameCodec.decode(badMagic).has_value(), "session command log frame codec should reject checksum-protected bad magic");

	dev::SessionCommandLogBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 2;
	dev::SessionCommandLogChecksum {}.appendTo(badVersion);
	Expect(!frameCodec.decode(badVersion).has_value(), "session command log frame codec should reject unsupported version");

	dev::SessionCommandLogBytes wrongCount = bytes;
	wrongCount.resize(wrongCount.size() - 4U);
	wrongCount[8] = 2;
	dev::SessionCommandLogChecksum {}.appendTo(wrongCount);
	Expect(!frameCodec.decode(wrongCount).has_value(), "session command log frame codec should reject payload size mismatch");
}

void TestSessionCommandLogFileStoreSavesLoadsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_replay_test";
	const std::filesystem::path path = root / "boot.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 4, 11 }, .playerHitPoints = 16 },
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Gameplay,
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session command log file store should save log bytes");
	std::optional<dev::SessionCommandLog> loaded = store.load(path);
	Expect(loaded.has_value(), "session command log file store should load saved log");
	Expect(loaded.has_value() && loaded->commands().size() == 3, "loaded session command log should preserve command count");

	dev::GameSession session { root / "saves" };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = loaded.has_value()
	    ? replayer.replay(*loaded)
	    : std::vector<dev::SessionCommandResult> {};

	Expect(results.size() == 3, "loaded session command log should replay");
	Expect(session.mode() == dev::GameSessionMode::Gameplay, "loaded session command log should reproduce final session mode");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 4, 11 }, "loaded session command log should reproduce player start");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 16, "loaded session command log should reproduce player hp");

	std::filesystem::remove_all(root);
}

void TestSessionCommandLogFileStoreRejectsCorruptAndMissingFiles()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_corrupt_test.iscl";
	const std::filesystem::path missingPath = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_missing_test.iscl";
	std::filesystem::remove(path);
	std::filesystem::remove(missingPath);

	{
		std::ofstream output { path, std::ios::binary | std::ios::trunc };
		output << "not a session command log";
	}

	dev::SessionCommandLogFileStore store;
	Expect(!store.load(path).has_value(), "session command log file store should reject corrupt files");
	Expect(!store.load(missingPath).has_value(), "session command log file store should return empty for missing files");

	std::filesystem::remove(path);
}

void TestSessionScriptRunnerRunsSavedLifecycleScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_script_runner_test";
	const std::filesystem::path path = root / "script.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 12, 6 }, .playerHitPoints = 21 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 2,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 2,
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session script runner test should create script file");

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionScriptRunner runner { dispatcher };
	dev::SessionScriptRunResult result = runner.run(path);

	Expect(result.status == dev::SessionScriptRunStatus::Completed, "session script runner should complete valid script files");
	Expect(result.commandResults.size() == 3, "session script runner should return per-command results");
	Expect(result.commandResults.size() == 3 && result.commandResults[0].type == dev::SessionCommandResultType::Applied, "session script runner should apply new-game command");
	Expect(result.commandResults.size() == 3 && result.commandResults[1].type == dev::SessionCommandResultType::Applied, "session script runner should apply save command");
	Expect(result.commandResults.size() == 3 && result.commandResults[2].type == dev::SessionCommandResultType::Applied, "session script runner should apply load command");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 12, 6 }, "session script runner should reproduce player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 21, "session script runner should reproduce player hp");
	Expect(events.events().size() == 3, "session script runner should still emit dispatcher events");

	std::filesystem::remove_all(root);
}

void TestSessionScriptRunnerReportsLoadFailureAndCommandRejectionSeparately()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_script_runner_failure_test";
	const std::filesystem::path path = root / "script.iscl";
	const std::filesystem::path missingPath = root / "missing.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameSession session { root / "saves" };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionScriptRunner runner { dispatcher };

	dev::SessionScriptRunResult missing = runner.run(missingPath);
	Expect(missing.status == dev::SessionScriptRunStatus::LoadFailed, "session script runner should report missing file load failure");
	Expect(missing.commandResults.empty(), "missing session script should not dispatch commands");

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session script runner rejection test should create script file");

	dev::SessionScriptRunResult rejected = runner.run(path);
	Expect(rejected.status == dev::SessionScriptRunStatus::Completed, "session script runner should complete loadable scripts even when commands reject");
	Expect(rejected.commandResults.size() == 1, "session script runner should return rejected command result");
	Expect(rejected.commandResults.size() == 1 && rejected.commandResults[0].type == dev::SessionCommandResultType::Rejected, "session script runner should preserve command-level rejection");

	std::filesystem::remove_all(root);
}

void TestRuntimeStartupScriptIntakeRunsLifecycleScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_startup_script_intake_test";
	const std::filesystem::path path = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 4, 5 }, .playerHitPoints = 19 },
	});
	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "runtime startup script intake test should create startup script");

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	const dev::SessionScriptRunResult result = dev::RuntimeStartupScriptIntake {}.run(path, dispatcher);

	Expect(result.status == dev::SessionScriptRunStatus::Completed, "runtime startup script intake should complete saved lifecycle scripts");
	Expect(result.commandResults.size() == 1 && result.commandResults[0].type == dev::SessionCommandResultType::Applied, "runtime startup script intake should dispatch lifecycle commands");
	Expect(session.mode() == dev::GameSessionMode::Gameplay, "runtime startup script intake should apply session mode changes");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 4, 5 }, "runtime startup script intake should update the session world");
	Expect(events.events().size() == 1, "runtime startup script intake should keep dispatcher event emission");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsStartupScriptAndFrames()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_script_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 2, 13 }, .playerHitPoints = 18 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "game loop startup test should create script file");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .frame = { .maxFrames = 2, .fixedDeltaSeconds = 1.0F / 30.0F },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptRan, "game loop should run configured startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::Completed, "game loop should report completed startup script");
	Expect(result.setup.startupScriptResult.commandResults.size() == 1, "game loop should expose startup command results");
	Expect(result.summary.framesRun == 2, "game loop should run configured frame count");
	Expect(result.finalMode == dev::GameSessionMode::Gameplay, "game loop should report final session mode");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 2, 13 }, "game loop startup script should initialize session world");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].combatStats.hitPoints == 18, "game loop startup script should initialize player hp");
	Expect(loop.sessionEvents().events().size() == 1, "game loop should keep startup session events observable");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsStartupScriptLoadFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_failure_test";
	const std::filesystem::path scriptPath = root / "missing.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .frame = { .maxFrames = 3 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptRan, "game loop should attempt configured startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "game loop should report startup script load failure");
	Expect(result.setup.startupScriptResult.commandResults.empty(), "failed startup script should not dispatch commands");
	Expect(result.summary.framesRun == 0, "game loop should not run frames after failed startup script");
	Expect(result.finalMode == dev::GameSessionMode::Empty, "failed startup script should leave session empty");
	Expect(loop.run() == 1, "game loop run should return failure exit code for missing startup script");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsInventoryScriptAgainstActivePlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_test";
	const std::filesystem::path scriptPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 970,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "game loop inventory script test should create script file");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .inventoryScript = scriptPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 970,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.inventoryScriptRan, "game loop should run configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::Completed, "game loop should report completed inventory script");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1, "game loop should expose inventory script command results");
	Expect(result.summary.inventoryCommandResults.size() == 1, "game loop should merge inventory script command results into loop results");
	Expect(loop.session().world().players[0].inventory.items.empty(), "game loop inventory script should remove equipped item from bag");
	Expect(loop.session().world().players[0].inventory.equipment.weapon.has_value() && loop.session().world().players[0].inventory.equipment.weapon->id == 970, "game loop inventory script should equip item");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Equipped, "game loop inventory script should emit inventory event");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsInventoryScriptAfterStartupScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_then_inventory_script_test";
	const std::filesystem::path startupPath = root / "startup.iscl";
	const std::filesystem::path inventoryPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog startup;
	startup.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 4, 9 }, .playerHitPoints = 16 },
	});
	dev::SessionCommandLogFileStore sessionStore;
	Expect(sessionStore.save(startupPath, startup), "startup then inventory test should create startup script");

	dev::InventoryCommandLog inventory;
	inventory.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 971,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(inventoryPath, inventory), "startup then inventory test should create inventory script");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = startupPath, .inventoryScript = inventoryPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptRan, "game loop should run startup script before inventory script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::Completed, "startup then inventory test should complete startup script");
	Expect(result.setup.inventoryScriptRan, "game loop should run inventory script after startup creates world");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::Completed, "inventory script should complete when startup created player");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1 && result.setup.inventoryScriptResult.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "inventory script should preserve command rejection after load");
	Expect(result.summary.framesRun == 1, "game loop should continue frames after loadable inventory script command rejects");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 4, 9 }, "startup should initialize world before inventory script");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].equipmentResult == dev::EquipmentResultType::MissingItem, "inventory script rejection should emit event");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsMovementScriptAfterStartupScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_then_movement_script_test";
	const std::filesystem::path startupPath = root / "startup.iscl";
	const std::filesystem::path movementPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog startup;
	startup.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});
	dev::SessionCommandLogFileStore sessionStore;
	Expect(sessionStore.save(startupPath, startup), "startup then movement test should create startup script");

	dev::CommandLog movement;
	movement.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore movementStore;
	Expect(movementStore.save(movementPath, movement), "startup then movement test should create movement script");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = startupPath, .movementScript = movementPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptRan, "game loop should run startup script before movement script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::Completed, "startup then movement test should complete startup script");
	Expect(result.setup.movementScriptRan, "game loop should run configured movement script after startup creates world");
	Expect(result.setup.movementScriptResult.status == dev::MovementScriptRunStatus::Completed, "movement setup script should complete when startup created world");
	Expect(result.setup.movementScriptResult.replayReport.acceptedCount() == 1, "movement setup script should report accepted movement command");
	Expect(result.summary.runtimeMovementScriptResults.empty(), "configured movement setup script should not be counted as runtime movement script source");
	Expect(result.summary.framesRun == 1, "game loop should run frames after configured movement script");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "configured movement setup script should feed first frame movement");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsInventoryScriptLoadFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_load_failure_test";
	const std::filesystem::path scriptPath = root / "missing.iicl";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .inventoryScript = scriptPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.inventoryScriptRan, "game loop should attempt configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::LoadFailed, "game loop should report inventory script load failure");
	Expect(result.setup.inventoryScriptResult.commandResults.empty(), "failed inventory script should not dispatch commands");
	Expect(result.summary.framesRun == 0, "game loop should stop before frames when configured inventory script cannot load");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsInventoryScriptWithoutActivePlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_no_player_test";
	const std::filesystem::path scriptPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog inventory;
	inventory.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 972,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, inventory), "no player inventory script test should create script file");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .inventoryScript = scriptPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.inventoryScriptRan, "game loop should notice configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::NoActivePlayer, "game loop should report inventory script without active player");
	Expect(result.setup.inventoryScriptResult.commandResults.empty(), "inventory script without player should not dispatch commands");
	Expect(result.summary.framesRun == 0, "game loop should stop before frames when inventory script has no active player");

	std::filesystem::remove_all(root);
}

void TestQueuedSessionCommandSourceDrainsCommandsOnce()
{
	dev::QueuedSessionCommandSource source;
	source.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 5, 5 } },
	});
	source.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});

	Expect(source.size() == 2, "queued session command source should track queued command count");
	std::vector<dev::SessionCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued session command source should drain queued commands");
	Expect(source.empty(), "queued session command source should be empty after drain");
	Expect(source.drain().empty(), "queued session command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].newGameSettings->playerStart == dev::Point { 5, 5 }, "queued session command source should preserve command payloads");
}

void TestQueuedInventoryCommandSourceDrainsCommandsOnce()
{
	dev::QueuedInventoryCommandSource source;
	source.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 940,
	});
	source.enqueue({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	Expect(source.size() == 2, "queued inventory command source should track queued command count");
	std::vector<dev::InventoryCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued inventory command source should drain queued commands");
	Expect(source.empty(), "queued inventory command source should be empty after drain");
	Expect(source.drain().empty(), "queued inventory command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].itemId == std::optional<dev::TargetId> { 940 }, "queued inventory command source should preserve equip payload");
	Expect(drained.size() == 2 && drained[1].slot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Weapon }, "queued inventory command source should preserve unequip payload");
}

void TestQueuedInventoryScriptSourceDrainsPathsOnce()
{
	dev::QueuedInventoryScriptSource source;
	const std::filesystem::path first = "equip.iicl";
	const std::filesystem::path second = "swap.iicl";
	source.enqueue(first);
	source.enqueue(second);

	Expect(source.size() == 2, "queued inventory script source should track queued path count");
	std::vector<std::filesystem::path> drained = source.drain();
	Expect(drained.size() == 2, "queued inventory script source should drain queued paths");
	Expect(source.empty(), "queued inventory script source should be empty after drain");
	Expect(source.drain().empty(), "queued inventory script source should not drain paths twice");
	Expect(drained.size() == 2 && drained[0] == first, "queued inventory script source should preserve first path");
	Expect(drained.size() == 2 && drained[1] == second, "queued inventory script source should preserve second path");
}

void TestQueuedMovementScriptSourceDrainsPathsOnce()
{
	dev::QueuedMovementScriptSource source;
	const std::filesystem::path first = "opening.imcl";
	const std::filesystem::path second = "combat.imcl";
	source.enqueue(first);
	source.enqueue(second);

	Expect(source.size() == 2, "queued movement script source should track queued path count");
	std::vector<std::filesystem::path> drained = source.drain();
	Expect(drained.size() == 2, "queued movement script source should drain queued paths");
	Expect(source.empty(), "queued movement script source should be empty after drain");
	Expect(source.drain().empty(), "queued movement script source should not drain paths twice");
	Expect(drained.size() == 2 && drained[0] == first, "queued movement script source should preserve first path");
	Expect(drained.size() == 2 && drained[1] == second, "queued movement script source should preserve second path");
	source.enqueue(first);
	source.clear();
	Expect(source.empty(), "queued movement script source should clear queued paths");
}

void TestRuntimeInventoryCommandIntakeDispatchesOrRejectsCommands()
{
	dev::Player player;
	player.inventory.items.push_back({
	    .id = 10,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;

	std::vector<dev::InventoryCommandResult> applied = dev::RuntimeInventoryCommandIntake {}.dispatch(
	    {
	        {
	            .type = dev::InventoryCommandType::EquipItem,
	            .itemId = 10,
	        },
	    },
	    &player,
	    &events);

	Expect(applied.size() == 1 && applied[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory command intake should dispatch commands for active player");
	Expect(player.inventory.items.empty(), "runtime inventory command intake should apply equipment changes");
	Expect(player.inventory.equipment.weapon.has_value() && player.inventory.equipment.weapon->id == 10, "runtime inventory command intake should equip item into slot");
	Expect(events.events().size() == 1 && events.events()[0].type == dev::InventoryEventType::Equipped, "runtime inventory command intake should emit applied inventory event");

	std::vector<dev::InventoryCommandResult> rejected = dev::RuntimeInventoryCommandIntake {}.dispatch(
	    {
	        {
	            .type = dev::InventoryCommandType::EquipItem,
	            .itemId = 11,
	        },
	    },
	    nullptr,
	    &events);

	Expect(rejected.size() == 1 && rejected[0].type == dev::InventoryCommandResultType::Rejected, "runtime inventory command intake should reject commands without active player");
	Expect(rejected.size() == 1 && rejected[0].command.itemId == std::optional<dev::TargetId> { 11 }, "runtime inventory command intake should preserve rejected command payload");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::InventoryEventType::Rejected, "runtime inventory command intake should emit rejected event without active player");
	Expect(events.events().size() == 2 && events.events()[1].commandType == dev::InventoryCommandType::EquipItem, "runtime inventory command intake should preserve rejected event command type");
}

void TestRuntimeSessionCommandIntakeDispatchesCommandsInOrder()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_session_command_intake_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	std::vector<dev::SessionCommandResult> results = dev::RuntimeSessionCommandIntake {}.dispatch(
	    {
	        {
	            .type = dev::SessionCommandType::StartNewGame,
	            .newGameSettings = dev::NewGameSettings { .playerStart = { 3, 4 }, .playerHitPoints = 12 },
	        },
	        {
	            .type = dev::SessionCommandType::SetMode,
	            .mode = dev::GameSessionMode::Inventory,
	        },
	    },
	    dispatcher);

	Expect(results.size() == 2, "runtime session command intake should dispatch every drained command");
	Expect(results.size() == 2 && results[0].command.type == dev::SessionCommandType::StartNewGame, "runtime session command intake should preserve first command order");
	Expect(results.size() == 2 && results[1].command.type == dev::SessionCommandType::SetMode, "runtime session command intake should preserve second command order");
	Expect(results.size() == 2 && results[0].type == dev::SessionCommandResultType::Applied, "runtime session command intake should apply start command");
	Expect(results.size() == 2 && results[1].type == dev::SessionCommandResultType::Applied, "runtime session command intake should apply mode command");
	Expect(session.hasActiveWorld(), "runtime session command intake should mutate session through dispatcher");
	Expect(session.mode() == dev::GameSessionMode::Inventory, "runtime session command intake should leave session in dispatched mode");
	Expect(events.events().size() == 2 && events.events()[0].commandType == dev::SessionCommandType::StartNewGame, "runtime session command intake should emit first lifecycle event");
	Expect(events.events().size() == 2 && events.events()[1].commandType == dev::SessionCommandType::SetMode, "runtime session command intake should emit second lifecycle event");

	std::filesystem::remove_all(root);
}

void TestRuntimeMovementScriptIntakeRunsScriptsAgainstActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_movement_script_intake_test";
	const std::filesystem::path scriptPath = root / "runtime_movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	log.record({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime movement script intake test should save script");

	dev::EventRecorder events;
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.movementEvents = &events;

	const dev::MovementScriptRunResult result = dev::RuntimeMovementScriptIntake {}.run(scriptPath, &world);
	const dev::MovementScriptRunResult missingWorld = dev::RuntimeMovementScriptIntake {}.run(scriptPath, nullptr);

	Expect(result.status == dev::MovementScriptRunStatus::Completed, "runtime movement script intake should run scripts against active world");
	Expect(result.replayReport.results.size() == 2, "runtime movement script intake should replay every command");
	Expect(result.replayReport.acceptedCount() == 2, "runtime movement script intake should report accepted replay commands");
	Expect(world.players[0].moveState == dev::PlayerMoveState::Idle, "runtime movement script intake should let script dispatch affect world player state");
	Expect(events.events().size() >= 2, "runtime movement script intake should emit movement events through the world sink");
	Expect(missingWorld.status == dev::MovementScriptRunStatus::NoActiveWorld, "runtime movement script intake should report missing active world");
	Expect(missingWorld.replayReport.results.empty(), "runtime movement script intake should not replay without active world");

	std::filesystem::remove_all(root);
}

void TestRuntimeMovementScriptBatchRunnerPreservesPathOrder()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_movement_script_batch_runner_test";
	const std::filesystem::path firstPath = root / "first.imcl";
	const std::filesystem::path missingPath = root / "missing.imcl";
	const std::filesystem::path thirdPath = root / "third.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog firstLog;
	firstLog.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLog thirdLog;
	thirdLog.record({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 0, 0 },
	});
	dev::CommandLogFileStore store;
	Expect(store.save(firstPath, firstLog), "runtime movement script batch runner test should save first script");
	Expect(store.save(thirdPath, thirdLog), "runtime movement script batch runner test should save third script");

	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	std::vector<dev::MovementScriptRunResult> results = dev::RuntimeMovementScriptBatchRunner {}.run(
	    { firstPath, missingPath, thirdPath },
	    world);

	Expect(results.size() == 3, "runtime movement script batch runner should produce one result per script path");
	Expect(results.size() == 3 && results[0].status == dev::MovementScriptRunStatus::Completed, "runtime movement script batch runner should keep first script result order");
	Expect(results.size() == 3 && results[1].status == dev::MovementScriptRunStatus::LoadFailed, "runtime movement script batch runner should keep missing script result order");
	Expect(results.size() == 3 && results[2].status == dev::MovementScriptRunStatus::Completed, "runtime movement script batch runner should continue after load failure");
	Expect(results.size() == 3 && results[0].replayReport.acceptedCount() == 1, "runtime movement script batch runner should replay first script");
	Expect(results.size() == 3 && results[2].replayReport.results.size() == 1, "runtime movement script batch runner should replay later scripts");

	std::filesystem::remove_all(root);
}

void TestRuntimeInventoryScriptIntakeRunsScriptsAgainstActivePlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_inventory_script_intake_test";
	const std::filesystem::path scriptPath = root / "runtime_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 12,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime inventory script intake test should save script");

	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 12,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;

	const dev::InventoryScriptRunResult result = dev::RuntimeInventoryScriptIntake {}.run(scriptPath, &player, &events);
	const dev::InventoryScriptRunResult missingPlayer = dev::RuntimeInventoryScriptIntake {}.run(scriptPath, nullptr, &events);

	Expect(result.status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script intake should run scripts against active player");
	Expect(result.commandResults.size() == 2, "runtime inventory script intake should replay every inventory command");
	Expect(result.commandResults.size() == 2 && result.commandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script intake should apply equip command");
	Expect(result.commandResults.size() == 2 && result.commandResults[1].type == dev::InventoryCommandResultType::Applied, "runtime inventory script intake should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "runtime inventory script intake should let script dispatch affect player equipment");
	Expect(events.events().size() == 2, "runtime inventory script intake should emit inventory events through provided sink");
	Expect(missingPlayer.status == dev::InventoryScriptRunStatus::NoActivePlayer, "runtime inventory script intake should report missing active player");
	Expect(missingPlayer.commandResults.empty(), "runtime inventory script intake should not replay without active player");
	Expect(events.events().size() == 2, "runtime inventory script intake should not emit extra events without active player");

	std::filesystem::remove_all(root);
}

void TestRuntimeInventoryScriptBatchRunnerPreservesPathOrder()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_inventory_script_batch_runner_test";
	const std::filesystem::path firstPath = root / "first.iicl";
	const std::filesystem::path missingPath = root / "missing.iicl";
	const std::filesystem::path thirdPath = root / "third.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog firstLog;
	firstLog.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 13,
	});
	dev::InventoryCommandLog thirdLog;
	thirdLog.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(firstPath, firstLog), "runtime inventory script batch runner test should save first script");
	Expect(store.save(thirdPath, thirdLog), "runtime inventory script batch runner test should save third script");

	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 13,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;

	std::vector<dev::InventoryScriptRunResult> results = dev::RuntimeInventoryScriptBatchRunner {}.run(
	    { firstPath, missingPath, thirdPath },
	    &player,
	    &events);

	Expect(results.size() == 3, "runtime inventory script batch runner should produce one result per script path");
	Expect(results.size() == 3 && results[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script batch runner should keep first script result order");
	Expect(results.size() == 3 && results[1].status == dev::InventoryScriptRunStatus::LoadFailed, "runtime inventory script batch runner should keep missing script result order");
	Expect(results.size() == 3 && results[2].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script batch runner should continue after load failure");
	Expect(results.size() == 3 && results[0].commandResults.size() == 1 && results[0].commandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script batch runner should replay first script");
	Expect(results.size() == 3 && results[2].commandResults.size() == 1 && results[2].commandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script batch runner should replay later scripts");
	Expect(events.events().size() == 2, "runtime inventory script batch runner should emit events for completed script commands only");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugManifestSetupTextFormatsSetupAttempts()
{
	dev::RuntimeSetupResult setup;
	setup.startupScriptRan = true;
	setup.inventoryScriptRan = false;
	setup.movementScriptRan = true;

	dev::RuntimeDebugManifestSetupText formatter;

	Expect(formatter.format(setup) == "setup startupScriptRan=true inventoryScriptRan=false movementScriptRan=true", "runtime debug manifest setup text should format setup attempt flags");
}

void TestRuntimeDebugManifestIndexTextFormatsBundleIndex()
{
	dev::RuntimeDebugManifestContext savedContext {
		.traceSaved = true,
	};
	dev::RuntimeDebugManifestContext failedContext {
		.traceSaved = false,
	};

	const std::vector<std::string> saved = dev::RuntimeDebugManifestIndexText {}.format(savedContext);
	const std::vector<std::string> failed = dev::RuntimeDebugManifestIndexText {}.format(failedContext);
	const std::vector<std::string> expectedSaved {
		"bundle version=1",
		"trace=run.trace saved=true",
	};
	const std::vector<std::string> expectedFailed {
		"bundle version=1",
		"trace=run.trace saved=false",
	};

	Expect(saved == expectedSaved, "runtime debug manifest index text should format saved trace index lines");
	Expect(failed == expectedFailed, "runtime debug manifest index text should format failed trace index lines");
}

void TestRuntimeDebugManifestPathsTextFormatsArtifactPaths()
{
	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
	};

	const std::vector<std::string> lines = dev::RuntimeDebugManifestPathsText {}.format(context);
	const std::vector<std::string> expected {
		"paths root=debug/run-001",
		"paths manifest=manifest.txt",
		"paths trace=run.trace",
	};

	Expect(lines == expected, "runtime debug manifest paths text should format artifact path lines");
}

void TestRuntimeDebugManifestSectionsFormatsRunStatus()
{
	dev::GameLoopResult result;
	result.summary.framesRun = 2;
	result.summary.movementInputBlockReasons.push_back(dev::PlayerActionBlockReason::Focus);
	result.summary.movementInputBlockReasons.push_back(dev::PlayerActionBlockReason::Paused);
	result.finalMode = dev::GameSessionMode::Inventory;

	const std::vector<std::string> lines = dev::RuntimeDebugManifestSections {}.formatRunStatus(result);
	const std::vector<std::string> expected {
		"run frames=2 frameReports=0 rawInput=0 movementInputBlocks=2 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0 finalMode=Inventory",
		"movementInputBlockReasons total=2 focus=1 paused=1 animationLocked=0 animationCommitment=0 stunned=0 none=0",
		"policy latest=none",
	};

	Expect(lines == expected, "runtime debug manifest sections should format run status lines in manifest order");
}

void TestRuntimeDebugManifestSectionsFormatsSetupDetails()
{
	dev::GameLoopResult result;
	result.setup.inventoryScriptRan = true;
	result.setup.inventoryScriptResult = {
		.status = dev::InventoryScriptRunStatus::Completed,
		.commandResults = {
		    { .type = dev::InventoryCommandResultType::Applied },
		},
	};
	result.setup.movementScriptRan = true;
	result.setup.movementScriptResult = {
		.status = dev::MovementScriptRunStatus::Completed,
		.replayReport = {
		    .results = {
		        { .type = dev::MovementCommandDispatchResultType::Accepted },
		    },
		},
	};

	const std::vector<std::string> lines = dev::RuntimeDebugManifestSections {}.formatSetup(result);
	const std::vector<std::string> expected {
		"setup startupScriptRan=false inventoryScriptRan=true movementScriptRan=true",
		"setup movementScript status=Completed results=1 accepted=1 rejected=0",
		"setup inventoryScript status=Completed results=1 applied=1 rejected=0",
	};

	Expect(lines == expected, "runtime debug manifest sections should format setup details in manifest order");
}

void TestRuntimeDebugManifestSectionsFormatsRuntimeScripts()
{
	dev::GameLoopResult result;
	result.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::LoadFailed,
	});
	result.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::NoActiveWorld,
	});

	const std::vector<std::string> lines = dev::RuntimeDebugManifestSections {}.formatRuntimeScripts(result);
	const std::vector<std::string> expected {
		"runtime inventoryScripts=1 completed=0 loadFailed=1 noActivePlayer=0 applied=0 rejected=0",
		"runtime movementScripts=1 completed=0 loadFailed=0 noActiveWorld=1 accepted=0 rejected=0",
	};

	Expect(lines == expected, "runtime debug manifest sections should format runtime script aggregate lines in manifest order");
}

void TestRuntimeFrameTraceFileStoreSavesAndLoadsLines()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_file_store_test";
	const std::filesystem::path path = root / "frame.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	std::vector<std::string> lines {
		"frame rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=1 inventoryResults=2 movementScripts=0 movementQueued=1",
		"inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=955 slot=Weapon",
		"movementEvent[0] type=CommandAccepted player=0 tile=(0,0) command=WalkTo",
	};

	dev::RuntimeFrameTraceFileStore store;
	Expect(store.save(path, lines), "runtime frame trace file store should save lines");
	std::optional<std::vector<std::string>> loaded = store.load(path);

	Expect(loaded.has_value(), "runtime frame trace file store should load saved lines");
	Expect(loaded.has_value() && *loaded == lines, "runtime frame trace file store should preserve exact lines");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "runtime frame trace file store should remove temp file after save");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameTraceFileStoreRejectsMissingFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_missing_test";
	const std::filesystem::path path = root / "missing.trace";
	std::filesystem::remove_all(root);

	dev::RuntimeFrameTraceFileStore store;
	Expect(!store.load(path).has_value(), "runtime frame trace file store should reject missing file");

	std::filesystem::remove_all(root);
}

void TestRuntimeTraceServiceFormatsAndSavesRunTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_trace_service_test";
	const std::filesystem::path scriptPath = root / "trace_inventory.iicl";
	const std::filesystem::path tracePath = root / "run.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 956,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(scriptPath, log), "runtime trace service test should create inventory script");

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryScriptSources = { &inventoryScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 956,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::GameLoopResult result = loop.runForResult();

	dev::RuntimeTraceService service;
	std::vector<std::string> lines = service.formatRun(result);
	Expect(service.saveRunTrace(tracePath, result), "runtime trace service should save full run trace");
	std::optional<std::vector<std::string>> loaded = dev::RuntimeFrameTraceFileStore {}.load(tracePath);

	Expect(!lines.empty(), "runtime trace service should format run lines");
	Expect(!lines.empty() && lines[0] == "run frames=1 frameReports=1 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=1 inventoryResults=1 movementScripts=0 movementQueued=0", "runtime trace service should include run summary");
	Expect(ContainsLineFragment(lines, "frame[0]"), "runtime trace service should include frame header");
	Expect(ContainsLineFragment(lines, "inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=956 slot=Weapon"), "runtime trace service should include frame trace detail");
	Expect(loaded.has_value() && *loaded == lines, "runtime trace service should persist exact formatted lines");

	std::filesystem::remove_all(root);
}

void TestRuntimeTraceServiceFormatsEmptyRun()
{
	dev::GameLoopResult result;
	std::vector<std::string> lines = dev::RuntimeTraceService {}.formatRun(result);

	Expect(lines.size() == 1, "runtime trace service should format empty run as summary only");
	Expect(lines.size() == 1 && lines[0] == "run frames=0 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime trace service should preserve empty run counts");
}

void TestRuntimeSetupSettingsDefaultsToNoScripts()
{
	dev::RuntimeSetupSettings setup;

	Expect(!setup.startupScript.has_value(), "runtime setup settings should default to no startup script");
	Expect(!setup.inventoryScript.has_value(), "runtime setup settings should default to no configured inventory script");
	Expect(!setup.movementScript.has_value(), "runtime setup settings should default to no configured movement script");
}

void TestRuntimeSetupResultDefaultsToNoSetupScripts()
{
	dev::RuntimeSetupResult setup;

	Expect(!setup.startupScriptRan, "runtime setup result should default to no startup script");
	Expect(setup.startupScriptResult.commandResults.empty(), "runtime setup result should default to no startup command results");
	Expect(!setup.inventoryScriptRan, "runtime setup result should default to no inventory script");
	Expect(setup.inventoryScriptResult.commandResults.empty(), "runtime setup result should default to no inventory command results");
	Expect(!setup.movementScriptRan, "runtime setup result should default to no movement script");
	Expect(setup.movementScriptResult.replayReport.results.empty(), "runtime setup result should default to no movement replay results");
}

void TestRuntimeSetupRunnerAllowsFramesWhenNoScriptsConfigured()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_empty_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({});

	Expect(result.framesAllowed, "runtime setup runner should allow frames when no setup scripts are configured");
	Expect(!result.setup.startupScriptRan, "runtime setup runner should not invent startup script attempts");
	Expect(!result.setup.inventoryScriptRan, "runtime setup runner should not invent inventory script attempts");
	Expect(!result.setup.movementScriptRan, "runtime setup runner should not invent movement script attempts");
	Expect(result.inventoryCommandResults.empty(), "runtime setup runner should report no setup inventory command results without scripts");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerStopsFramesAfterStartupLoadFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_startup_failure_test";
	const std::filesystem::path missingStartup = root / "missing.iscl";
	const std::filesystem::path missingInventory = root / "missing.iicl";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .startupScript = missingStartup,
	    .inventoryScript = missingInventory,
	});

	Expect(!result.framesAllowed, "runtime setup runner should stop frames after startup load failure");
	Expect(result.setup.startupScriptRan, "runtime setup runner should attempt configured startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "runtime setup runner should report startup load failure");
	Expect(!result.setup.inventoryScriptRan, "runtime setup runner should not run inventory setup after startup load failure");
	Expect(!result.setup.movementScriptRan, "runtime setup runner should not run movement setup after startup load failure");
	Expect(result.inventoryCommandResults.empty(), "failed startup setup should not produce inventory command results");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerPreservesInventoryCommandRejections()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_inventory_rejection_test";
	const std::filesystem::path inventoryPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog inventoryLog;
	inventoryLog.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 72,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(inventoryPath, inventoryLog), "runtime setup runner inventory rejection test should create inventory script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .inventoryScript = inventoryPath,
	});

	Expect(result.framesAllowed, "runtime setup runner should allow frames after loadable inventory script command rejection");
	Expect(result.setup.inventoryScriptRan, "runtime setup runner should attempt configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::Completed, "runtime setup runner should complete loadable inventory scripts");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1, "runtime setup runner should preserve inventory script command results");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1 && result.setup.inventoryScriptResult.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup runner should preserve rejected inventory command result");
	Expect(result.inventoryCommandResults.size() == 1 && result.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup runner should expose configured inventory command results for run summaries");
	Expect(inventoryEvents.events().size() == 1 && inventoryEvents.events()[0].type == dev::InventoryEventType::Rejected, "runtime setup runner should preserve inventory setup events");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerStopsFramesAfterInventorySetupFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_inventory_failure_test";
	const std::filesystem::path missingInventory = root / "missing.iicl";
	const std::filesystem::path movementPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog movementLog;
	movementLog.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore movementStore;
	Expect(movementStore.save(movementPath, movementLog), "runtime setup runner inventory failure test should create movement script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .inventoryScript = missingInventory,
	    .movementScript = movementPath,
	});

	Expect(!result.framesAllowed, "runtime setup runner should stop frames after inventory setup failure");
	Expect(result.setup.inventoryScriptRan, "runtime setup runner should attempt configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::LoadFailed, "runtime setup runner should report inventory setup load failure");
	Expect(!result.setup.movementScriptRan, "runtime setup runner should not run movement setup after inventory setup failure");
	Expect(result.inventoryCommandResults.empty(), "failed inventory setup should not expose setup inventory command results");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerPreservesMovementCommandRejections()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_movement_rejection_test";
	const std::filesystem::path movementPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog movementLog;
	movementLog.record({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});
	dev::CommandLogFileStore movementStore;
	Expect(movementStore.save(movementPath, movementLog), "runtime setup runner movement rejection test should create movement script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .movementScript = movementPath,
	});

	Expect(result.framesAllowed, "runtime setup runner should allow frames after loadable movement script command rejection");
	Expect(result.setup.movementScriptRan, "runtime setup runner should attempt configured movement script");
	Expect(result.setup.movementScriptResult.status == dev::MovementScriptRunStatus::Completed, "runtime setup runner should complete loadable movement scripts");
	Expect(result.setup.movementScriptResult.replayReport.results.size() == 1, "runtime setup runner should preserve movement replay results");
	Expect(result.setup.movementScriptResult.replayReport.rejectedCount() == 1, "runtime setup runner should preserve rejected movement commands");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerStopsFramesWithoutActiveWorldForMovementScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_movement_no_world_test";
	const std::filesystem::path movementPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog movementLog;
	movementLog.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore movementStore;
	Expect(movementStore.save(movementPath, movementLog), "runtime setup runner no-world movement test should create movement script");

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .movementScript = movementPath,
	});

	Expect(!result.framesAllowed, "runtime setup runner should stop frames when movement setup has no active world");
	Expect(result.setup.movementScriptRan, "runtime setup runner should attempt configured movement script without world");
	Expect(result.setup.movementScriptResult.status == dev::MovementScriptRunStatus::NoActiveWorld, "runtime setup runner should report movement setup without active world");

	std::filesystem::remove_all(root);
}

void TestRuntimeSourceSettingsDefaultsToNoSources()
{
	dev::RuntimeSourceSettings sources;

	Expect(sources.rawInputSources.empty(), "runtime source settings should default to no raw input sources");
	Expect(sources.sessionCommandSources.empty(), "runtime source settings should default to no session command sources");
	Expect(sources.movementCommandSources.empty(), "runtime source settings should default to no movement command sources");
	Expect(sources.inventoryCommandSources.empty(), "runtime source settings should default to no inventory command sources");
	Expect(sources.inventoryScriptSources.empty(), "runtime source settings should default to no inventory script sources");
}

void TestRuntimeInputSettingsDefaultsToPrimaryGameplayInput()
{
	dev::RuntimeInputSettings input;

	Expect(input.bindings.pauseKey == 27, "runtime input settings should default pause binding to escape");
	Expect(input.bindings.inventoryKey == 'I', "runtime input settings should default inventory binding to I");
	Expect(input.bindings.stopKey == 'S', "runtime input settings should default stop binding to S");
	Expect(input.focusState.owner == dev::InputOwner::Gameplay, "runtime input settings should default focus to gameplay");
	Expect(!input.focusState.textEntryActive, "runtime input settings should default to no text entry");
	Expect(!input.actionContext.paused, "runtime input settings should default actions to unpaused");
	Expect(!input.actionContext.animationLocked, "runtime input settings should default actions to unlocked animation");
	Expect(input.playerId == 0, "runtime input settings should default to player zero");
	Expect(input.targetResolver == nullptr, "runtime input settings should default to world target resolver fallback");
}

void TestRuntimeInputContextBuilderHandlesMissingWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_input_context_no_world_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::RuntimeInputContext context = dev::RuntimeInputContextBuilder { session }.build({
	    .focusState = dev::FocusState { .owner = dev::InputOwner::Inventory },
	    .actionContext = dev::PlayerActionContext { .paused = true },
	    .playerId = 2,
	});

	Expect(context.world == nullptr, "runtime input context builder should not expose a world before one exists");
	Expect(context.playerId == 2, "runtime input context builder should preserve configured player id");
	Expect(context.focusState.owner == dev::InputOwner::Inventory, "runtime input context builder should preserve focus state");
	Expect(context.actionContext.paused, "runtime input context builder should preserve action context");
	Expect(context.sessionMode == dev::GameSessionMode::Empty, "runtime input context builder should expose current session mode");
	Expect(context.targetResolver == nullptr, "runtime input context builder should not invent a target resolver without a world");

	std::filesystem::remove_all(root);
}

void TestRuntimeInputContextBuilderUsesWorldTargetsUnlessOverridden()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_input_context_world_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::RuntimeInputContext worldContext = dev::RuntimeInputContextBuilder { session }.build({});

	FixedTargetResolver explicitTargets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 91,
		    .tile = { 0, 0 },
		}
	};
	dev::RuntimeInputContext overrideContext = dev::RuntimeInputContextBuilder { session }.build({
	    .targetResolver = &explicitTargets,
	});

	Expect(worldContext.world == &session.world(), "runtime input context builder should expose active world");
	Expect(worldContext.sessionMode == dev::GameSessionMode::Gameplay, "runtime input context builder should expose gameplay session mode");
	Expect(worldContext.targetResolver == &session.world().targets, "runtime input context builder should use world targets by default");
	Expect(overrideContext.targetResolver == &explicitTargets, "runtime input context builder should preserve explicit target resolver override");

	std::filesystem::remove_all(root);
}

void TestRuntimeInputRouteResultBuilderNamesRouteOutcomes()
{
	dev::RuntimeInputRouteResultBuilder builder;

	const dev::RuntimeInputRouteResult unhandled = builder.unhandled();
	const dev::RuntimeInputRouteResult session = builder.queuedSessionCommand();
	const dev::RuntimeInputRouteResult movement = builder.queuedMovementCommand();
	const dev::RuntimeInputRouteResult blocked = builder.blockedMovement(dev::PlayerActionBlockReason::Focus);

	Expect(!unhandled.handled && !unhandled.queuedSessionCommand && !unhandled.queuedMovementCommand && !unhandled.movementBlockReason.has_value(), "runtime input route result builder should name unhandled events");
	Expect(session.handled && session.queuedSessionCommand && !session.queuedMovementCommand && !session.movementBlockReason.has_value(), "runtime input route result builder should name queued session commands");
	Expect(movement.handled && !movement.queuedSessionCommand && movement.queuedMovementCommand && !movement.movementBlockReason.has_value(), "runtime input route result builder should name queued movement commands");
	Expect(!blocked.handled && !blocked.queuedSessionCommand && !blocked.queuedMovementCommand, "runtime input route result builder should keep blocked movement unhandled");
	Expect(blocked.movementBlockReason == std::optional<dev::PlayerActionBlockReason> { dev::PlayerActionBlockReason::Focus }, "runtime input route result builder should preserve blocked movement reason");
}

void TestRuntimeInventoryScriptReportRecorderKeepsScriptsAndFlattensCommands()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 10 },
	});
	summary.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 11 },
	});

	dev::RuntimeInventoryScriptReportRecorder {}.record(
	    {
	        {
	            .status = dev::InventoryScriptRunStatus::Completed,
	            .commandResults = {
	                {
	                    .type = dev::InventoryCommandResultType::Applied,
	                    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 12 },
	                    .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 12 },
	                },
	            },
	        },
	    },
	    frame,
	    summary);

	Expect(frame.inventoryScriptResults.size() == 1 && frame.inventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script report recorder should copy script results onto frame report");
	Expect(summary.runtimeInventoryScriptResults.size() == 1 && summary.runtimeInventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script report recorder should aggregate script results onto summary");
	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 10 }, "runtime inventory script report recorder should preserve existing frame command results");
	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[1].command.itemId == std::optional<dev::TargetId> { 12 }, "runtime inventory script report recorder should append script command results to frame");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 11 }, "runtime inventory script report recorder should preserve existing summary command results");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[1].command.itemId == std::optional<dev::TargetId> { 12 }, "runtime inventory script report recorder should append script command results to summary");
}

void TestRuntimeInventoryCommandReportRecorderAppendsDirectCommandResults()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 20 },
	});
	summary.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 21 },
	});

	dev::RuntimeInventoryCommandReportRecorder {}.record(
	    {
	        {
	            .type = dev::InventoryCommandResultType::Rejected,
	            .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
	            .equipmentResult = { .type = dev::EquipmentResultType::MissingItem },
	        },
	    },
	    frame,
	    summary);

	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 20 }, "runtime inventory command report recorder should preserve existing frame command results");
	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[1].command.type == dev::InventoryCommandType::UnequipSlot, "runtime inventory command report recorder should append direct command result to frame");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 21 }, "runtime inventory command report recorder should preserve existing summary command results");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[1].command.type == dev::InventoryCommandType::UnequipSlot, "runtime inventory command report recorder should append direct command result to summary");
}

void TestRuntimeSessionCommandReportRecorderReplacesFrameAndAggregatesSummary()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.sessionCommandResults.push_back({
	    .type = dev::SessionCommandResultType::Rejected,
	    .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Paused },
	});
	summary.sessionCommandResults.push_back({
	    .type = dev::SessionCommandResultType::Applied,
	    .command = { .type = dev::SessionCommandType::StartNewGame },
	});

	dev::RuntimeSessionCommandReportRecorder {}.record(
	    {
	        {
	            .type = dev::SessionCommandResultType::Applied,
	            .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Inventory },
	        },
	    },
	    frame,
	    summary);

	Expect(frame.sessionCommandResults.size() == 1 && frame.sessionCommandResults[0].command.mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "runtime session command report recorder should replace frame session results with current frame results");
	Expect(summary.sessionCommandResults.size() == 2 && summary.sessionCommandResults[0].command.type == dev::SessionCommandType::StartNewGame, "runtime session command report recorder should preserve existing summary session results");
	Expect(summary.sessionCommandResults.size() == 2 && summary.sessionCommandResults[1].command.mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "runtime session command report recorder should append current frame results to summary");
}

void TestRuntimeMovementScriptReportRecorderReplacesFrameAndAggregatesSummary()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.movementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::LoadFailed,
	});
	summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::NoActiveWorld,
	});

	dev::RuntimeMovementScriptReportRecorder {}.record(
	    {
	        {
	            .status = dev::MovementScriptRunStatus::Completed,
	        },
	    },
	    frame,
	    summary);

	Expect(frame.movementScriptResults.size() == 1 && frame.movementScriptResults[0].status == dev::MovementScriptRunStatus::Completed, "runtime movement script report recorder should replace frame movement scripts with current frame results");
	Expect(summary.runtimeMovementScriptResults.size() == 2 && summary.runtimeMovementScriptResults[0].status == dev::MovementScriptRunStatus::NoActiveWorld, "runtime movement script report recorder should preserve existing summary movement script results");
	Expect(summary.runtimeMovementScriptResults.size() == 2 && summary.runtimeMovementScriptResults[1].status == dev::MovementScriptRunStatus::Completed, "runtime movement script report recorder should append current frame movement script results");
	Expect(summary.movementCommandsQueued == 0, "runtime movement script report recorder should not inflate queued movement command count");
}

void TestRuntimeSetupInventoryCommandReportRecorderAppendsOnlySummaryResults()
{
	dev::RuntimeRunSummary summary;
	summary.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 1 },
	    .equipmentResult = { .type = dev::EquipmentResultType::MissingItem, .itemId = 1 },
	});

	dev::RuntimeSetupInventoryCommandReportRecorder {}.record(
	    {
	        {
	            .type = dev::InventoryCommandResultType::Applied,
	            .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 9 },
	            .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 9 },
	        },
	    },
	    summary);

	Expect(summary.inventoryCommandResults.size() == 2, "runtime setup inventory command report recorder should append setup results to existing summary results");
	Expect(summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup inventory command report recorder should preserve existing summary result order");
	Expect(summary.inventoryCommandResults[1].command.itemId == std::optional<dev::TargetId> { 9 }, "runtime setup inventory command report recorder should preserve setup command payload");
	Expect(summary.framesRun == 0, "runtime setup inventory command report recorder should not count frames");
}

void TestRuntimeSetupRunResultApplierCopiesSetupAndRecordsSummary()
{
	dev::RuntimeSetupRunResult setupResult;
	setupResult.setup.startupScriptRan = true;
	setupResult.setup.inventoryScriptRan = true;
	setupResult.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 12 },
	    .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 12 },
	});
	setupResult.framesAllowed = false;

	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	const bool framesAllowed = dev::RuntimeSetupRunResultApplier {}.apply(
	    setupResult,
	    result,
	    recorder);

	Expect(!framesAllowed, "runtime setup run result applier should return setup frame gate");
	Expect(result.setup.startupScriptRan, "runtime setup run result applier should copy startup setup result");
	Expect(result.setup.inventoryScriptRan, "runtime setup run result applier should copy inventory setup result");
	Expect(result.summary.inventoryCommandResults.size() == 1, "runtime setup run result applier should record setup inventory command results");
	Expect(result.summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 12 }, "runtime setup run result applier should preserve setup command payload");
	Expect(result.summary.framesRun == 0, "runtime setup run result applier should not finish frames");
}

void TestRuntimeFrameSettingsDefaultsToNoFramesAtSixtyHz()
{
	dev::RuntimeFrameSettings frame;

	Expect(frame.maxFrames == 0, "runtime frame settings should default to zero bounded frames");
	Expect(frame.fixedDeltaSeconds == 1.0F / 60.0F, "runtime frame settings should default to sixty hertz timestep");
}

void TestRuntimeRunSummaryDefaultsToEmptyRun()
{
	dev::RuntimeRunSummary summary;

	Expect(summary.runtimeInventoryScriptResults.empty(), "runtime run summary should default to no runtime inventory script results");
	Expect(summary.rawInputEventsRouted == 0, "runtime run summary should default to no routed raw input");
	Expect(summary.movementInputBlockReasons.empty(), "runtime run summary should default to no movement input block reasons");
	Expect(summary.sessionCommandResults.empty(), "runtime run summary should default to no session command results");
	Expect(summary.inventoryCommandResults.empty(), "runtime run summary should default to no inventory command results");
	Expect(summary.movementCommandsQueued == 0, "runtime run summary should default to no queued movement commands");
	Expect(summary.framesRun == 0, "runtime run summary should default to zero frames");
	Expect(summary.lastFrameEvents.movementEvents().empty(), "runtime run summary should default to no final movement events");
}

void TestRuntimeRunRecorderAggregatesSetupInventoryResultsWithoutFrame()
{
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	recorder.recordSetupInventoryCommandResults({
	    {
	        .type = dev::InventoryCommandResultType::Applied,
	        .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 73 },
	        .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 73 },
	    },
	});

	Expect(result.summary.inventoryCommandResults.size() == 1, "runtime run recorder should aggregate setup inventory command results");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 73 }, "runtime run recorder should preserve setup inventory result payload");
	Expect(result.summary.framesRun == 0, "runtime run recorder setup aggregation should not count a frame");
	Expect(result.frameReports.empty(), "runtime run recorder setup aggregation should not create a frame report");
}

void TestRuntimeRunRecorderAggregatesFrameReportsAndSummary()
{
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	sessionEvents.emit({
	    .type = dev::SessionEventType::GameStarted,
	    .commandType = dev::SessionCommandType::StartNewGame,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Rejected,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Rejected,
	    .equipmentResult = dev::EquipmentResultType::MissingItem,
	    .itemId = 70,
	});

	recorder.beginFrame();
	sessionEvents.emit({
	    .type = dev::SessionEventType::ModeChanged,
	    .commandType = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Equipped,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Applied,
	    .equipmentResult = dev::EquipmentResultType::Equipped,
	    .itemId = 71,
	});

	recorder.recordRawInputDrainResult({
	    .handled = 2,
	    .movementBlockReasons = { dev::PlayerActionBlockReason::Focus },
	});
	recorder.recordSessionCommandResults({
	    {
	        .type = dev::SessionCommandResultType::Applied,
	        .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Inventory },
	    },
	});
	recorder.recordInventoryScriptResults({
	    {
	        .status = dev::InventoryScriptRunStatus::Completed,
	        .commandResults = {
	            {
	                .type = dev::InventoryCommandResultType::Applied,
	                .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 71 },
	                .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 71 },
	            },
	        },
	    },
	});
	recorder.recordInventoryCommandResults({
	    {
	        .type = dev::InventoryCommandResultType::Applied,
	        .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
	        .equipmentResult = { .type = dev::EquipmentResultType::Unequipped, .itemId = 71 },
	    },
	});
	recorder.recordMovementCommandsQueued(1);
	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit({
	    .type = dev::MovementEventType::CommandAccepted,
	    .playerId = 0,
	    .tile = { 1, 0 },
	    .commandType = dev::MovementCommandType::WalkTo,
	});
	recorder.recordFrameEvents(frameEvents);
	recorder.finishFrame();

	Expect(result.summary.framesRun == 1, "runtime run recorder should count finished frames");
	Expect(result.summary.rawInputEventsRouted == 2, "runtime run recorder should aggregate routed raw input");
	Expect(result.summary.movementInputBlockReasons.size() == 1 && result.summary.movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "runtime run recorder should aggregate movement input block reasons");
	Expect(result.summary.sessionCommandResults.size() == 1, "runtime run recorder should aggregate session results");
	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "runtime run recorder should aggregate inventory script results");
	Expect(result.summary.inventoryCommandResults.size() == 2, "runtime run recorder should aggregate script and direct inventory results");
	Expect(result.summary.movementCommandsQueued == 1, "runtime run recorder should aggregate movement queue counts");
	Expect(result.summary.lastFrameEvents.movementEvents().size() == 1, "runtime run recorder should store final frame events");
	Expect(result.frameReports.size() == 1, "runtime run recorder should create one frame report");
	if (result.frameReports.empty())
		return;

	const dev::RuntimeFrameReport &report = result.frameReports[0];
	Expect(report.rawInputEventsRouted == 2, "runtime run recorder frame report should keep raw input count");
	Expect(report.movementInputBlockReasons.size() == 1 && report.movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "runtime run recorder frame report should keep movement input block reasons");
	Expect(report.sessionCommandResults.size() == 1, "runtime run recorder frame report should keep session results");
	Expect(report.inventoryScriptResults.size() == 1, "runtime run recorder frame report should keep inventory scripts");
	Expect(report.inventoryCommandResults.size() == 2, "runtime run recorder frame report should keep script and direct inventory results");
	Expect(report.movementCommandsQueued == 1, "runtime run recorder frame report should keep movement queue count");
	Expect(report.sessionEvents.size() == 1 && report.sessionEvents[0].type == dev::SessionEventType::ModeChanged, "runtime run recorder should capture session event deltas");
	Expect(report.inventoryEvents.size() == 1 && report.inventoryEvents[0].itemId == 71, "runtime run recorder should capture inventory event deltas");
	Expect(report.frameEvents.movementEvents().size() == 1, "runtime run recorder frame report should keep simulation frame events");
}

void TestRuntimeSetupFrameGateAllowsOnlyNonFatalSetupResults()
{
	dev::RuntimeSetupFrameGate gate;

	dev::RuntimeSetupResult clean;

	dev::RuntimeSetupResult commandRejection;
	commandRejection.inventoryScriptRan = true;
	commandRejection.inventoryScriptResult.status = dev::InventoryScriptRunStatus::Completed;
	commandRejection.inventoryScriptResult.commandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 4 },
	    .equipmentResult = { .type = dev::EquipmentResultType::MissingItem, .itemId = 4 },
	});

	dev::RuntimeSetupResult startupFailure;
	startupFailure.startupScriptRan = true;
	startupFailure.startupScriptResult.status = dev::SessionScriptRunStatus::LoadFailed;

	dev::RuntimeSetupResult movementFailure;
	movementFailure.movementScriptRan = true;
	movementFailure.movementScriptResult.status = dev::MovementScriptRunStatus::NoActiveWorld;

	Expect(gate.allowsFrames(clean), "runtime setup frame gate should allow frames after clean setup");
	Expect(gate.allowsFrames(commandRejection), "runtime setup frame gate should allow frames after command-level setup rejections");
	Expect(!gate.allowsFrames(startupFailure), "runtime setup frame gate should block frames after startup load failure");
	Expect(!gate.allowsFrames(movementFailure), "runtime setup frame gate should block frames after movement setup failure");
}

void TestRuntimeDebugArtifactBundleSavesManifestAndTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_bundle_test";
	const std::filesystem::path bundleRoot = root / "bundle";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::GameLoopResult run = loop.runForResult();

	dev::RuntimeDebugArtifactBundle bundle;
	dev::RuntimeDebugArtifactBundleResult result = bundle.save(bundleRoot, run);
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "run.trace");

	Expect(result.rootPrepared, "runtime debug bundle should create bundle root");
	Expect(result.traceSaved, "runtime debug bundle should save run trace");
	Expect(result.manifestSaved, "runtime debug bundle should save manifest");
	Expect(result.saved(), "runtime debug bundle should report complete save");
	Expect(result.manifestPath == bundleRoot / "manifest.txt", "runtime debug bundle should use stable manifest path");
	Expect(result.tracePath == bundleRoot / "run.trace", "runtime debug bundle should use stable trace path");
	Expect(manifest.has_value(), "runtime debug bundle manifest should be loadable text");
	Expect(trace.has_value(), "runtime debug bundle trace should be loadable text");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "bundle version=1"), "runtime debug bundle manifest should include version");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "runtime debug bundle manifest should index trace artifact");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "run frames=1 frameReports=1"), "runtime debug bundle manifest should summarize run frame counts");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "finalMode=Gameplay"), "runtime debug bundle manifest should include final mode");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "policy latest=Gameplay acceptCommands=true updatePlayers=true updateEnemies=true"), "runtime debug bundle manifest should summarize latest frame policy");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "runtime inventoryScripts=0 completed=0 loadFailed=0 noActivePlayer=0 applied=0 rejected=0"), "runtime debug bundle manifest should summarize runtime inventory scripts");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "runtime movementScripts=0 completed=0 loadFailed=0 noActiveWorld=0 accepted=0 rejected=0"), "runtime debug bundle manifest should summarize runtime movement scripts");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=1 frameReports=1 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime debug bundle trace should preserve run trace summary");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugManifestFormatsFailedRun()
{
	dev::GameLoopResult run;
	run.setup.startupScriptRan = true;
	run.finalMode = dev::GameSessionMode::Empty;

	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
		.traceSaved = false,
	};

	std::vector<std::string> lines = dev::RuntimeDebugManifest {}.format(run, context);

	Expect(ContainsLineFragment(lines, "trace=run.trace saved=false"), "runtime debug bundle manifest should report trace save state");
	Expect(ContainsLineFragment(lines, "run frames=0 frameReports=0"), "runtime debug bundle manifest should summarize empty failed runs");
	Expect(ContainsLineFragment(lines, "finalMode=Empty"), "runtime debug bundle manifest should name empty final mode");
	Expect(ContainsLineFragment(lines, "policy latest=none"), "runtime debug bundle manifest should report no frame policy for zero-frame runs");
	Expect(ContainsLineFragment(lines, "setup startupScriptRan=true inventoryScriptRan=false movementScriptRan=false"), "runtime debug bundle manifest should report setup attempts");
	Expect(ContainsLineFragment(lines, "runtime inventoryScripts=0 completed=0 loadFailed=0 noActivePlayer=0 applied=0 rejected=0"), "runtime debug bundle manifest should report empty runtime inventory scripts");
	Expect(ContainsLineFragment(lines, "runtime movementScripts=0 completed=0 loadFailed=0 noActiveWorld=0 accepted=0 rejected=0"), "runtime debug bundle manifest should report empty runtime movement scripts");
}

void TestRuntimeDebugManifestSummarizesInventoryScripts()
{
	dev::GameLoopResult run;
	run.setup.inventoryScriptRan = true;
	run.setup.inventoryScriptResult = {
		.status = dev::InventoryScriptRunStatus::Completed,
		.commandResults = {
		    {
		        .type = dev::InventoryCommandResultType::Applied,
		        .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 10 },
		    },
		    {
		        .type = dev::InventoryCommandResultType::Rejected,
		        .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
		    },
		},
	};
	run.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::Completed,
	    .commandResults = {
	        {
	            .type = dev::InventoryCommandResultType::Applied,
	            .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 11 },
	        },
	    },
	});
	run.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::LoadFailed,
	});
	run.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::NoActivePlayer,
	});
	run.finalMode = dev::GameSessionMode::Gameplay;

	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-inventory",
		.manifestPath = "debug/run-inventory/manifest.txt",
		.tracePath = "debug/run-inventory/run.trace",
		.traceSaved = true,
	};

	std::vector<std::string> lines = dev::RuntimeDebugManifest {}.format(run, context);

	Expect(ContainsLineFragment(lines, "setup startupScriptRan=false inventoryScriptRan=true movementScriptRan=false"), "runtime debug manifest should report configured inventory setup attempt");
	Expect(ContainsLineFragment(lines, "setup inventoryScript status=Completed results=2 applied=1 rejected=1"), "runtime debug manifest should summarize configured inventory setup result");
	Expect(ContainsLineFragment(lines, "runtime inventoryScripts=3 completed=1 loadFailed=1 noActivePlayer=1 applied=1 rejected=0"), "runtime debug manifest should summarize runtime inventory script results");
}

void TestRuntimeDebugManifestSummarizesMovementScripts()
{
	dev::GameLoopResult run;
	run.setup.movementScriptRan = true;
	run.setup.movementScriptResult = {
		.status = dev::MovementScriptRunStatus::Completed,
		.replayReport = {
		    .results = {
		        {
		            .type = dev::MovementCommandDispatchResultType::Accepted,
		            .command = {
		                .type = dev::MovementCommandType::WalkTo,
		                .playerId = 0,
		                .destination = { 1, 0 },
		            },
		        },
		        {
		            .type = dev::MovementCommandDispatchResultType::Rejected,
		            .command = {
		                .type = dev::MovementCommandType::MoveThenAct,
		                .playerId = 0,
		                .destination = { 1, 0 },
		                .destinationAction = std::nullopt,
		            },
		        },
		    },
		},
	};
	run.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::Completed,
	    .replayReport = {
	        .results = {
	            {
	                .type = dev::MovementCommandDispatchResultType::Accepted,
	                .command = {
	                    .type = dev::MovementCommandType::WalkTo,
	                    .playerId = 0,
	                    .destination = { 1, 0 },
	                },
	            },
	        },
	    },
	});
	run.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::LoadFailed,
	});
	run.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::NoActiveWorld,
	});
	run.finalMode = dev::GameSessionMode::Gameplay;

	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-002",
		.manifestPath = "debug/run-002/manifest.txt",
		.tracePath = "debug/run-002/run.trace",
		.traceSaved = true,
	};

	std::vector<std::string> lines = dev::RuntimeDebugManifest {}.format(run, context);

	Expect(ContainsLineFragment(lines, "setup startupScriptRan=false inventoryScriptRan=false movementScriptRan=true"), "runtime debug manifest should report configured movement setup attempt");
	Expect(ContainsLineFragment(lines, "setup movementScript status=Completed results=2 accepted=1 rejected=1"), "runtime debug manifest should summarize configured movement setup replay result");
	Expect(ContainsLineFragment(lines, "runtime movementScripts=3 completed=1 loadFailed=1 noActiveWorld=1 accepted=1 rejected=0"), "runtime debug manifest should summarize runtime movement script results");
}

void TestRuntimeDebugArtifactLayoutNamesBundlePaths()
{
	const std::filesystem::path root = "debug/run-001";

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);

	Expect(paths.rootPath == root, "runtime debug artifact layout should preserve root path");
	Expect(paths.manifestPath == root / "manifest.txt", "runtime debug artifact layout should name manifest path");
	Expect(paths.tracePath == root / "run.trace", "runtime debug artifact layout should name trace path");
}

void TestRuntimeDebugArtifactBundleResultBuilderRecordsBundleState()
{
	const dev::RuntimeDebugArtifactPaths paths {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
	};
	dev::RuntimeDebugArtifactBundleResultBuilder builder { paths };

	dev::RuntimeDebugArtifactBundleResult initial = builder.result();
	Expect(initial.rootPath == paths.rootPath, "runtime debug artifact bundle result builder should copy root path");
	Expect(initial.manifestPath == paths.manifestPath, "runtime debug artifact bundle result builder should copy manifest path");
	Expect(initial.tracePath == paths.tracePath, "runtime debug artifact bundle result builder should copy trace path");
	Expect(!initial.rootPrepared && !initial.traceSaved && !initial.manifestSaved, "runtime debug artifact bundle result builder should default to unsaved state");
	Expect(!initial.saved(), "runtime debug artifact bundle result builder should not report saved before writes");

	builder.markRootPrepared();
	builder.recordWrite({ .traceSaved = true, .manifestSaved = true });
	dev::RuntimeDebugArtifactBundleResult saved = builder.result();

	Expect(saved.rootPrepared, "runtime debug artifact bundle result builder should record prepared root");
	Expect(saved.traceSaved && saved.manifestSaved, "runtime debug artifact bundle result builder should record write flags");
	Expect(saved.saved(), "runtime debug artifact bundle result builder should report complete bundle save");
}

void TestRuntimeDebugArtifactRootPreparerCreatesBundleRoot()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_root_preparer_test";
	std::filesystem::remove_all(root);

	bool prepared = dev::RuntimeDebugArtifactRootPreparer {}.prepare(root / "nested" / "bundle");

	Expect(prepared, "runtime debug artifact root preparer should create missing bundle directories");
	Expect(std::filesystem::is_directory(root / "nested" / "bundle"), "runtime debug artifact root preparer should leave a directory at bundle root");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactRootPreparerRejectsRootFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_root_preparer_file_test";
	std::filesystem::remove_all(root);
	{
		std::ofstream output { root, std::ios::trunc };
		output << "not a directory\n";
	}

	bool prepared = dev::RuntimeDebugArtifactRootPreparer {}.prepare(root);

	Expect(!prepared, "runtime debug artifact root preparer should reject existing files");

	std::filesystem::remove(root);
}

void TestRuntimeDebugManifestContextBuilderMapsPathsAndTraceState()
{
	const dev::RuntimeDebugArtifactPaths paths {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
	};

	dev::RuntimeDebugManifestContext context = dev::RuntimeDebugManifestContextBuilder {}.build(paths, true);

	Expect(context.rootPath == paths.rootPath, "runtime debug manifest context builder should copy bundle root path");
	Expect(context.manifestPath == paths.manifestPath, "runtime debug manifest context builder should copy manifest path");
	Expect(context.tracePath == paths.tracePath, "runtime debug manifest context builder should copy trace path");
	Expect(context.traceSaved, "runtime debug manifest context builder should copy trace save state");
}

void TestRuntimeDebugTraceWriteStepSavesBundleTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_trace_write_step_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult run;
	run.summary.framesRun = 4;

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);
	const bool saved = dev::RuntimeDebugTraceWriteStep {}.write(paths, run);
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(paths.tracePath);

	Expect(saved, "runtime debug trace write step should save run trace");
	Expect(trace.has_value(), "runtime debug trace write step should write readable trace");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=4 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime debug trace write step should preserve run summary");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugManifestWriteStepSavesManifestWithTraceState()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_manifest_write_step_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult run;
	run.finalMode = dev::GameSessionMode::Gameplay;
	run.summary.framesRun = 1;

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);
	const bool saved = dev::RuntimeDebugManifestWriteStep {}.write(paths, run, false);
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(paths.manifestPath);

	Expect(saved, "runtime debug manifest write step should save manifest lines");
	Expect(manifest.has_value(), "runtime debug manifest write step should write readable manifest");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=false"), "runtime debug manifest write step should record trace save state");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "root="), "runtime debug manifest write step should include artifact paths");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "run frames=1"), "runtime debug manifest write step should include run frame count");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "finalMode=Gameplay"), "runtime debug manifest write step should include final mode");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactWriterSavesTraceAndManifest()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_writer_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult run;
	run.finalMode = dev::GameSessionMode::Gameplay;
	run.summary.framesRun = 2;

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);
	dev::RuntimeDebugArtifactWriteResult result = dev::RuntimeDebugArtifactWriter {}.write(paths, run);
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(paths.manifestPath);
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(paths.tracePath);

	Expect(result.traceSaved, "runtime debug artifact writer should save trace");
	Expect(result.manifestSaved, "runtime debug artifact writer should save manifest");
	Expect(manifest.has_value(), "runtime debug artifact writer should write readable manifest");
	Expect(trace.has_value(), "runtime debug artifact writer should write readable trace");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "runtime debug artifact writer manifest should record saved trace");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "policy latest=none"), "runtime debug artifact writer manifest should report no frame policy without frame reports");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=2 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime debug artifact writer should preserve trace summary");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactWriterRecordsTraceFailureInManifest()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_writer_trace_failure_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::RuntimeDebugArtifactPaths paths {
		.rootPath = root,
		.manifestPath = root / "manifest.txt",
		.tracePath = root / "missing-parent" / "run.trace",
	};

	dev::RuntimeDebugArtifactWriteResult result = dev::RuntimeDebugArtifactWriter {}.write(paths, dev::GameLoopResult {});
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(paths.manifestPath);

	Expect(!result.traceSaved, "runtime debug artifact writer should report trace save failure");
	Expect(result.manifestSaved, "runtime debug artifact writer should still save manifest after trace failure");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=false"), "runtime debug artifact writer manifest should record failed trace");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactBundleRejectsRootFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_bundle_root_file_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root.parent_path());
	{
		std::ofstream output { root, std::ios::trunc };
		output << "not a directory\n";
	}

	dev::RuntimeDebugArtifactBundleResult result = dev::RuntimeDebugArtifactBundle {}.save(root, dev::GameLoopResult {});

	Expect(!result.rootPrepared, "runtime debug bundle should reject a root path that is already a file");
	Expect(!result.traceSaved, "runtime debug bundle should not save trace when root cannot be prepared");
	Expect(!result.manifestSaved, "runtime debug bundle should not save manifest when root cannot be prepared");
	Expect(!result.saved(), "runtime debug bundle should report incomplete save on root failure");

	std::filesystem::remove(root);
}

void TestRuntimeInputRouterMapsHotkeysToSessionCommands()
{
	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router {
		sessionCommands,
		movementCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'B', .stopKey = 'X' },
	};

	dev::RuntimeInputRouteResult pauseResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'P',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	dev::RuntimeInputRouteResult inventoryResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'B',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(pauseResult.handled && pauseResult.queuedSessionCommand, "runtime input router should map pause hotkey to session command");
	Expect(inventoryResult.handled && inventoryResult.queuedSessionCommand, "runtime input router should map inventory hotkey to session command");
	Expect(sessionCommands.size() == 2, "runtime input router should queue session hotkey commands");
	Expect(movementCommands.empty(), "runtime input router should not queue movement for session hotkeys");

	std::vector<dev::SessionCommand> commands = sessionCommands.drain();
	Expect(commands.size() == 2 && commands[0].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Paused }, "pause hotkey should request paused mode");
	Expect(commands.size() == 2 && commands[1].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "inventory hotkey should request inventory mode");
}

void TestRuntimeSessionInputRouterTogglesLifecycleModes()
{
	dev::QueuedSessionCommandSource sessionCommands;
	dev::RuntimeSessionInputRouter router {
		sessionCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'B', .stopKey = 'S' },
	};

	dev::RuntimeInputRouteResult pauseResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'P',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .sessionMode = dev::GameSessionMode::Paused,
	    });
	dev::RuntimeInputRouteResult inventoryResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'B',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .sessionMode = dev::GameSessionMode::Inventory,
	    });

	Expect(pauseResult.handled && pauseResult.queuedSessionCommand, "runtime session input router should handle pause key");
	Expect(inventoryResult.handled && inventoryResult.queuedSessionCommand, "runtime session input router should handle inventory key");
	std::vector<dev::SessionCommand> commands = sessionCommands.drain();
	Expect(commands.size() == 2 && commands[0].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Gameplay }, "pause key should unpause when already paused");
	Expect(commands.size() == 2 && commands[1].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Gameplay }, "inventory key should close inventory when already in inventory mode");
}

void TestRuntimeSessionModeTogglePolicyMapsHotkeysToRequestedModes()
{
	dev::RuntimeSessionModeTogglePolicy policy;

	Expect(policy.togglePause(dev::GameSessionMode::Gameplay) == dev::GameSessionMode::Paused, "runtime session mode toggle policy should pause gameplay");
	Expect(policy.togglePause(dev::GameSessionMode::Paused) == dev::GameSessionMode::Gameplay, "runtime session mode toggle policy should unpause paused mode");
	Expect(policy.togglePause(dev::GameSessionMode::Inventory) == dev::GameSessionMode::Paused, "runtime session mode toggle policy should let pause key request paused mode from inventory");
	Expect(policy.toggleInventory(dev::GameSessionMode::Gameplay) == dev::GameSessionMode::Inventory, "runtime session mode toggle policy should open inventory from gameplay");
	Expect(policy.toggleInventory(dev::GameSessionMode::Inventory) == dev::GameSessionMode::Gameplay, "runtime session mode toggle policy should close inventory");
	Expect(policy.toggleInventory(dev::GameSessionMode::Paused) == dev::GameSessionMode::Inventory, "runtime session mode toggle policy should let inventory key request inventory mode from pause");
}

void TestRuntimeInputRouterMapsStopHotkeyToMovementCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 7, 4 }));

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router {
		sessionCommands,
		movementCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'Q' },
	};

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'Q',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime input router should map stop hotkey to movement command");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::Stop, "stop hotkey should request Stop movement command");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 7, 4 }, "stop hotkey should use current player tile");
	Expect(sessionCommands.empty(), "stop hotkey should not queue session commands");
}

void TestRuntimeStopMovementInputStepMapsStopHotkeyToCommand()
{
	dev::Player player = MakePlayer({ 7, 4 });
	dev::InputFocus focus { dev::FocusState { .owner = dev::InputOwner::Gameplay } };
	dev::PlayerActionGate gate {
		focus,
		dev::PlayerActionContext {},
	};
	dev::QueuedMovementCommandSource movementCommands;

	dev::RuntimeInputRouteResult result = dev::RuntimeStopMovementInputStep { movementCommands }.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'Q',
	        .pressed = true,
	    },
	    'Q',
	    0,
	    player,
	    gate,
	    dev::PlayerActionBlockReason::None);

	Expect(result.handled && result.queuedMovementCommand, "runtime stop movement input step should handle stop hotkeys");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::Stop, "runtime stop movement input step should build Stop commands");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 7, 4 }, "runtime stop movement input step should use the current player tile");
}

void TestRuntimeMovementInputRouterReportsBlockedStopReason()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 7, 4 }));

	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeMovementInputRouter router {
		movementCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'Q' },
	};

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'Q',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .actionContext = { .paused = true },
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(!result.handled, "runtime movement input router should not handle blocked stop hotkey");
	Expect(result.movementBlockReason == std::optional<dev::PlayerActionBlockReason> { dev::PlayerActionBlockReason::Paused }, "runtime movement input router should report paused stop block reason");
	Expect(movementCommands.empty(), "blocked stop hotkey should not queue movement command");
}

} // namespace

int main()
{
	TestInventoryFocusBlocksMovement();
	TestPlayerActionGateReportsMovementBlockReasons();
	TestInputEventMatcherRecognizesPressedKeysAndPointers();
	TestStandGroundCreatesStandAndAct();
	TestDestinationActionBuilderMapsInteractionRanges();
	TestMoveThenActExecutesAfterPath();
	TestPlayerPathPlannerStartsPathAndEvents();
	TestMovementCommandValidatorRequiresActionPayloads();
	TestCommandDispatcherRejectsInvalidActionCommand();
	TestDiagonalCornerPolicyBlocksCornerCutting();
	TestActionExecutorWaitsOutOfRange();
	TestMoveThenActEventSequence();
	TestActorStepCommitterCommitsActorPosition();
	TestPlayerPathStepperReportsActionReady();
	TestPlayerAnimationLockGateBlocksUntilCancelWindow();
	TestPlayerActionRunnerExecutesReadyDestinationAction();
	TestCommandReplayProducesSameEventSequence();
	TestCommandReplayReportsRejectedCommands();
	TestMovementCodecRoundTrip();
	TestCommandLogCodecRoundTripsAndReplays();
	TestCommandLogCodecRejectsInvalidBytes();
	TestCommandLogChecksumValidatesTrailingChecksum();
	TestCommandPacketListCodecFramesPacketBytes();
	TestCommandPacketListCodecRejectsInvalidSizes();
	TestCommandLogFrameCodecFramesPacketBytes();
	TestCommandLogFrameCodecRejectsInvalidFrames();
	TestCommandLogFileStoreSavesLoadsAndReplays();
	TestCommandLogFileStoreRejectsCorruptAndMissingFiles();
	TestMovementScriptRunnerRunsSavedMovementScript();
	TestMovementScriptRunnerDistinguishesLoadFailureFromCommandRejection();
	TestEnemyPursuitStepPlannerChoosesNextTileTowardTarget();
	TestEnemyPursuitStepGateRequiresWalkableUnblockedTile();
	TestEnemyAttackRangeUsesEnemyTuning();
	TestEnemyAttackEntryPolicyUsesRange();
	TestEnemyPursuitBudgetUsesMaxStepsPerTick();
	TestEnemyPursuitObeysStepBudget();
	TestEnemyPursuitStepperStopsAtAttackRange();
	TestEnemyPursuitStepperReportsBudgetAndBlockedStops();
	TestEnemyPursuitStepperReportsAlreadyAtTarget();
	TestEnemyPursuitEventEmitterRecordsStopReason();
	TestEnemyMovementEmitsPursuitResult();
	TestEnemyAttackWindupAndRecovery();
	TestEnemyAttackRunnerConsumesWindupAndRecovery();
	TestEnemyAttackRunnerReportsRecoveryCompletedOutOfRange();
	TestEnemyAttackPhaseRunnerAdvancesWindupAndRecovery();
	TestEnemyAttackPhaseRunnerStartsWindup();
	TestEnemyAttackRestartPolicyUsesRecoveredRange();
	TestEnemyAttackEventEmitterRecordsTransitions();
	TestEnemyMovementEmitsAttackTransition();
	TestEnemyMovementReporterPublishesAttackAndPursuit();
	TestCombatResolverDamageAndDefeat();
	TestActionExecutorAttackResolvesCombat();
	TestCombatSystemEmitsHitEvent();
	TestCombatSystemEmitsDefeatedEvent();
	TestPlayerAttackUsesEquippedCombatModifiers();
	TestEnemyAttackUsesEquippedDefenseModifiers();
	TestEnemyAttackResolvesCombatAgainstPlayer();
	TestSimulationPlayerUpdaterAdvancesPlayerMovement();
	TestSimulationPlayerMovementRunnerWiresWorldServices();
	TestSimulationEnemyTargetSelectorChoosesCurrentPlayer();
	TestSimulationEnemyMovementRunnerWiresWorldServices();
	TestSimulationEnemyUpdaterAdvancesEnemyMovement();
	TestSimulationEnemyUpdaterSkipsWithoutTarget();
	TestSimulationActorUpdaterRunsPlayersBeforeEnemies();
	TestSimulationActorUpdaterCanSkipEnemies();
	TestSimulationTickPipelineDrainsCommandsBeforeActors();
	TestSimulationTickPipelineCanSkipCommandIntake();
	TestSimulationTickDispatchesMovementAndCombat();
	TestSimulationPolicyPausedDoesNotDrainCommands();
	TestSimulationFramePolicyDescriberReportsModePolicy();
	TestSimulationClockHitStopFreezesActorUpdates();
	TestSimulationClockScalesEnemyWindup();
	TestEffectRouterMapsMovementEventsToRequests();
	TestEffectRouterMapsCombatHitToRequests();
	TestEffectApplierAppliesHitStopToClock();
	TestSimulationTimeStepBuilderUsesClock();
	TestSimulationEffectPipelineRoutesAndAppliesEffects();
	TestSimulationEffectFinalizerRunsEffectConsequences();
	TestSimulationFrameEventCaptureCollectsForwardsAndRestoresSinks();
	TestSimulationFrameFinalizerAppliesConsequences();
	TestSimulationFrameTickRunnerCollectsTickEventsAndRestoresSinks();
	TestSimulationTargetFinalizerSynchronizesTargets();
	TestSimulationInventoryFinalizerAppliesPickupConsequences();
	TestSimulationFrameRunnerProcessesConsequences();
	TestTargetRegistryResolvesAndRemovesTargets();
	TestTargetSynchronizerSyncsEnemyTargetsWithoutRemovingObjects();
	TestTargetSynchronizerSkipsDefeatedEnemies();
	TestFrameRunnerSynchronizesMovedEnemyTargets();
	TestFrameRunnerRemovesDefeatedEnemyTargets();
	TestWorldEntityServiceSpawnsEnemyStateAcrossRegistries();
	TestWorldEntityServiceDespawnsEnemyStateAcrossRegistries();
	TestWorldEntityServiceRespawnReplacesStaleEnemyState();
	TestWorldEntityServiceSpawnsAndDespawnsItems();
	TestWorldEntityServiceRespawnReplacesStaleItemTarget();
	TestInventoryServiceTransfersExecutedPickupToPlayerInventory();
	TestInventoryServiceIgnoresInvalidPickupEvents();
	TestInventoryServiceRejectsPickupWhenInventoryIsFull();
	TestEquipmentServiceEquipsItemFromInventory();
	TestEquipmentServiceSwapsOccupiedSlot();
	TestEquipmentServiceRejectsMissingAndNotEquippableItems();
	TestEquipmentServiceUnequipsWhenInventoryHasCapacity();
	TestEquipmentServiceRejectsUnequipWhenInventoryFull();
	TestEquipmentStatsServiceBuildsEffectiveCombatStats();
	TestInventoryCommandDispatcherEquipsItem();
	TestInventoryCommandDispatcherUnequipsSlot();
	TestInventoryCommandDispatcherRejectsInvalidCommands();
	TestInventoryCommandDispatcherEmitsInventoryEvents();
	TestInventoryCommandEventEmitterMapsResultsToEvents();
	TestInventoryCommandCodecRoundTripsCommands();
	TestInventoryCommandCodecRejectsInvalidPackets();
	TestInventoryCommandPacketValidatorRejectsMalformedPayloads();
	TestInventoryCommandByteStreamWritesLittleEndianPrimitives();
	TestInventoryCommandByteStreamRejectsShortReads();
	TestInventoryCommandPacketByteCodecRoundTripsPackets();
	TestInventoryCommandPacketByteCodecRejectsInvalidBytes();
	TestInventoryCommandLogReplaysThroughDispatcher();
	TestInventoryCommandLogCodecRoundTripsAndReplays();
	TestInventoryCommandLogCodecRejectsInvalidBytes();
	TestInventoryCommandLogChecksumValidatesTrailingChecksum();
	TestInventoryCommandPacketListCodecFramesPacketBytes();
	TestInventoryCommandPacketListCodecRejectsInvalidSizes();
	TestInventoryCommandLogFrameCodecFramesPacketBytes();
	TestInventoryCommandLogFrameCodecRejectsInvalidFrames();
	TestByteFileStoreSavesLoadsAndCleansTempFile();
	TestByteFileStoreRejectsMissingAndUnwritablePaths();
	TestTextFileStoreSavesLoadsAndCleansTempFile();
	TestTextFileStoreRejectsMissingAndUnwritablePaths();
	TestInventoryCommandLogFileStoreSavesLoadsAndReplays();
	TestInventoryCommandLogFileStoreRejectsCorruptAndMissingFiles();
	TestInventoryScriptRunnerRunsSavedInventoryScript();
	TestInventoryScriptRunnerReportsLoadFailureAndCommandRejectionSeparately();
	TestSimulationSnapshotRestoresDurableState();
	TestSnapshotCodecRoundTripsVersionedBytes();
	TestSnapshotCodecRejectsInvalidBytes();
	TestSnapshotChecksumValidatesTrailingChecksum();
	TestSnapshotByteStreamWritesLittleEndianPrimitives();
	TestSnapshotByteStreamRejectsShortReads();
	TestSnapshotEntityCodecRoundTripsItemAndCombatant();
	TestSnapshotEntityCodecRejectsInvalidEnums();
	TestSnapshotPlayerCodecRoundTripsDurablePlayerState();
	TestSnapshotPlayerCodecRejectsInvalidMoveState();
	TestSnapshotEnemyCodecRoundTripsDurableEnemyState();
	TestSnapshotEnemyCodecRejectsInvalidMoveState();
	TestSnapshotVectorCodecFramesCountedVectors();
	TestSnapshotVectorCodecRejectsTruncatedVectors();
	TestSnapshotSchemaCodecRoundTripsOrderedSections();
	TestSnapshotSchemaCodecRejectsIncompleteOrTrailingPayload();
	TestSnapshotFrameCodecFramesPayloadBytes();
	TestSnapshotFrameCodecRejectsInvalidFrames();
	TestSnapshotFileStoreSavesAndLoadsVersionedBytes();
	TestSnapshotFileStoreRejectsCorruptFile();
	TestSaveGameServiceSavesAndLoadsWorld();
	TestSaveSlotServiceListsMetadata();
	TestSaveSlotServiceLoadsWorld();
	TestGameSessionStartsNewGameAndUpdates();
	TestNewGameWorldBuilderCreatesPlayerWorld();
	TestSessionWorldSlotLoaderLoadsWorldPreservingSinks();
	TestSessionWorldSlotSaverRequiresActiveSession();
	TestSessionFrameUpdaterAppliesModePolicy();
	TestGameSessionPausedModePreservesCommands();
	TestSessionModePolicyMapsModesToFramePolicy();
	TestSessionModePolicyGuardsTransitions();
	TestSessionModeChangerAppliesAllowedTransitionsOnly();
	TestGameSessionSaveLoadPreservesSinksAndResetsClock();
	TestGameSessionMissingLoadKeepsCurrentWorld();
	TestSessionCommandDispatcherAppliesLifecycleCommands();
	TestSessionCommandDispatcherRejectsInvalidLifecycleCommands();
	TestSessionCommandApplierMapsLifecycleOutcomes();
	TestSessionCommandApplierMapsRejectedOutcomes();
	TestSessionEventEmitterBuildsLifecycleEvents();
	TestSessionCommandDispatcherEmitsSuccessEvents();
	TestSessionCommandDispatcherEmitsFailureEvents();
	TestSessionCommandReplayAppliesLifecycleSequence();
	TestSessionCommandReplayReportsRejectedCommands();
	TestSessionCommandCodecRoundTripsCommands();
	TestSessionCommandCodecRejectsInvalidPackets();
	TestSessionCommandPacketValidatorRejectsMalformedPayloads();
	TestSessionCommandByteStreamWritesLittleEndianPrimitives();
	TestSessionCommandByteStreamRejectsShortReads();
	TestSessionCommandPacketByteCodecRoundTripsPackets();
	TestSessionCommandPacketByteCodecRejectsInvalidBytes();
	TestSessionCommandLogCodecRoundTripsAndReplays();
	TestSessionCommandLogCodecRejectsInvalidBytes();
	TestSessionCommandLogChecksumValidatesTrailingChecksum();
	TestSessionCommandPacketListCodecFramesPacketBytes();
	TestSessionCommandPacketListCodecRejectsInvalidSizes();
	TestSessionCommandLogFrameCodecFramesPacketBytes();
	TestSessionCommandLogFrameCodecRejectsInvalidFrames();
	TestSessionCommandLogFileStoreSavesLoadsAndReplays();
	TestSessionCommandLogFileStoreRejectsCorruptAndMissingFiles();
	TestSessionScriptRunnerRunsSavedLifecycleScript();
	TestSessionScriptRunnerReportsLoadFailureAndCommandRejectionSeparately();
	TestRuntimeStartupScriptIntakeRunsLifecycleScript();
	TestGameLoopRunsStartupScriptAndFrames();
	TestGameLoopReportsStartupScriptLoadFailure();
	TestGameLoopRunsInventoryScriptAgainstActivePlayer();
	TestGameLoopRunsInventoryScriptAfterStartupScript();
	TestGameLoopRunsMovementScriptAfterStartupScript();
	TestGameLoopReportsInventoryScriptLoadFailure();
	TestGameLoopReportsInventoryScriptWithoutActivePlayer();
	TestQueuedSessionCommandSourceDrainsCommandsOnce();
	TestQueuedInventoryCommandSourceDrainsCommandsOnce();
	TestQueuedInventoryScriptSourceDrainsPathsOnce();
	TestQueuedMovementScriptSourceDrainsPathsOnce();
	TestRuntimeInventoryCommandIntakeDispatchesOrRejectsCommands();
	TestRuntimeSessionCommandIntakeDispatchesCommandsInOrder();
	TestRuntimeMovementScriptIntakeRunsScriptsAgainstActiveWorld();
	TestRuntimeMovementScriptBatchRunnerPreservesPathOrder();
	TestRuntimeInventoryScriptIntakeRunsScriptsAgainstActivePlayer();
	TestRuntimeInventoryScriptBatchRunnerPreservesPathOrder();
	TestRuntimeDebugManifestSetupTextFormatsSetupAttempts();
	TestRuntimeDebugManifestIndexTextFormatsBundleIndex();
	TestRuntimeDebugManifestPathsTextFormatsArtifactPaths();
	TestRuntimeDebugManifestSectionsFormatsRunStatus();
	TestRuntimeDebugManifestSectionsFormatsSetupDetails();
	TestRuntimeDebugManifestSectionsFormatsRuntimeScripts();
	TestRuntimeFrameTraceFileStoreSavesAndLoadsLines();
	TestRuntimeFrameTraceFileStoreRejectsMissingFile();
	TestRuntimeTraceServiceFormatsAndSavesRunTrace();
	TestRuntimeTraceServiceFormatsEmptyRun();
	TestRuntimeSetupSettingsDefaultsToNoScripts();
	TestRuntimeSetupResultDefaultsToNoSetupScripts();
	TestRuntimeSetupRunnerAllowsFramesWhenNoScriptsConfigured();
	TestRuntimeSetupRunnerStopsFramesAfterStartupLoadFailure();
	TestRuntimeSetupRunnerPreservesInventoryCommandRejections();
	TestRuntimeSetupRunnerStopsFramesAfterInventorySetupFailure();
	TestRuntimeSetupRunnerPreservesMovementCommandRejections();
	TestRuntimeSetupRunnerStopsFramesWithoutActiveWorldForMovementScript();
	TestRuntimeSourceSettingsDefaultsToNoSources();
	TestRuntimeInputSettingsDefaultsToPrimaryGameplayInput();
	TestRuntimeInputContextBuilderHandlesMissingWorld();
	TestRuntimeInputContextBuilderUsesWorldTargetsUnlessOverridden();
	TestRuntimeInputRouteResultBuilderNamesRouteOutcomes();
	TestRuntimeInventoryScriptReportRecorderKeepsScriptsAndFlattensCommands();
	TestRuntimeInventoryCommandReportRecorderAppendsDirectCommandResults();
	TestRuntimeSessionCommandReportRecorderReplacesFrameAndAggregatesSummary();
	TestRuntimeMovementScriptReportRecorderReplacesFrameAndAggregatesSummary();
	TestRuntimeSetupInventoryCommandReportRecorderAppendsOnlySummaryResults();
	TestRuntimeSetupRunResultApplierCopiesSetupAndRecordsSummary();
	TestRuntimeFrameSettingsDefaultsToNoFramesAtSixtyHz();
	TestRuntimeRunSummaryDefaultsToEmptyRun();
	TestRuntimeRunRecorderAggregatesSetupInventoryResultsWithoutFrame();
	TestRuntimeRunRecorderAggregatesFrameReportsAndSummary();
	TestRuntimeSetupFrameGateAllowsOnlyNonFatalSetupResults();
	TestRuntimeDebugArtifactBundleSavesManifestAndTrace();
	TestRuntimeDebugManifestFormatsFailedRun();
	TestRuntimeDebugManifestSummarizesInventoryScripts();
	TestRuntimeDebugManifestSummarizesMovementScripts();
	TestRuntimeDebugArtifactLayoutNamesBundlePaths();
	TestRuntimeDebugArtifactBundleResultBuilderRecordsBundleState();
	TestRuntimeDebugArtifactRootPreparerCreatesBundleRoot();
	TestRuntimeDebugArtifactRootPreparerRejectsRootFile();
	TestRuntimeDebugManifestContextBuilderMapsPathsAndTraceState();
	TestRuntimeDebugTraceWriteStepSavesBundleTrace();
	TestRuntimeDebugManifestWriteStepSavesManifestWithTraceState();
	TestRuntimeDebugArtifactWriterSavesTraceAndManifest();
	TestRuntimeDebugArtifactWriterRecordsTraceFailureInManifest();
	TestRuntimeDebugArtifactBundleRejectsRootFile();
	TestRuntimeInputRouterMapsHotkeysToSessionCommands();
	TestRuntimeSessionInputRouterTogglesLifecycleModes();
	TestRuntimeSessionModeTogglePolicyMapsHotkeysToRequestedModes();
	TestRuntimeInputRouterMapsStopHotkeyToMovementCommand();
	TestRuntimeStopMovementInputStepMapsStopHotkeyToCommand();
	TestRuntimeMovementInputRouterReportsBlockedStopReason();

	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "movement_tests passed\n";
	return EXIT_SUCCESS;
}
