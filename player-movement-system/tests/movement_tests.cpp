#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "actions/ActionExecutor.hpp"
#include "commands/CommandDispatcher.hpp"
#include "enemies/EnemyMovement.hpp"
#include "events/EventRecorder.hpp"
#include "focus/InputFocus.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
#include "network/MovementCodec.hpp"
#include "player/PlayerActionGate.hpp"
#include "player/PlayerController.hpp"
#include "player/PlayerMovement.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandReplayer.hpp"
#include "targeting/Target.hpp"
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
	enemy.position.tile = tile;
	enemy.position.future = tile;
	enemy.position.previous = tile;
	enemy.position.precise = tile;
	return enemy;
}

void TestInventoryFocusBlocksMovement()
{
	dev::FocusState focusState { .owner = dev::InputOwner::Inventory };
	dev::InputFocus focus { focusState };
	dev::PlayerActionContext context;
	dev::PlayerActionGate gate { focus, context };
	dev::Player player = MakePlayer();

	Expect(!gate.canMove(player), "inventory focus should block movement");
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

	replayer.replay(log);
	movement.update(players, 0.016F);

	const std::vector<dev::MovementEvent> &recorded = events.events();
	Expect(recorded.size() >= 6, "replayed command should produce movement/action events");
	Expect(recorded[0].type == dev::MovementEventType::CommandAccepted, "replay should accept command");
	Expect(recorded[1].type == dev::MovementEventType::PathStarted, "replay should start path");
	Expect(recorded[2].type == dev::MovementEventType::StepCommitted, "replay should commit step");
	Expect(recorded[5].type == dev::MovementEventType::ActionExecuted, "replay should execute action");
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

} // namespace

int main()
{
	TestInventoryFocusBlocksMovement();
	TestStandGroundCreatesStandAndAct();
	TestMoveThenActExecutesAfterPath();
	TestDiagonalCornerPolicyBlocksCornerCutting();
	TestActionExecutorWaitsOutOfRange();
	TestMoveThenActEventSequence();
	TestCommandReplayProducesSameEventSequence();
	TestMovementCodecRoundTrip();
	TestEnemyPursuitObeysStepBudget();
	TestEnemyAttackWindupAndRecovery();

	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "movement_tests passed\n";
	return EXIT_SUCCESS;
}
