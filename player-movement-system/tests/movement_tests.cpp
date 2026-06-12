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
#include "focus/InputFocus.hpp"
#include "interaction/DestinationActionBuilder.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
#include "inventory/InventoryCommandCodec.hpp"
#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandEventEmitter.hpp"
#include "inventory/InventoryCommandByteStream.hpp"
#include "inventory/InventoryCommandPacketByteCodec.hpp"
#include "inventory/InventoryCommandPacketValidator.hpp"
#include "inventory/InventoryEventRecorder.hpp"
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
	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "movement_tests passed\n";
	return EXIT_SUCCESS;
}
