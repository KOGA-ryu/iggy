#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "actions/ActionExecutor.hpp"
#include "commands/CommandDispatcher.hpp"
#include "focus/InputFocus.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
#include "player/PlayerActionGate.hpp"
#include "player/PlayerController.hpp"
#include "player/PlayerMovement.hpp"
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

} // namespace

int main()
{
	TestInventoryFocusBlocksMovement();
	TestStandGroundCreatesStandAndAct();
	TestMoveThenActExecutesAfterPath();
	TestDiagonalCornerPolicyBlocksCornerCutting();
	TestActionExecutorWaitsOutOfRange();

	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "movement_tests passed\n";
	return EXIT_SUCCESS;
}
