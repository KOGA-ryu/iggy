#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "app/RuntimeBlockedPointerInputStep.hpp"
#include "app/RuntimeInputFocusResolver.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeMovementInputContextBuilder.hpp"
#include "app/RuntimeMovementInputRouter.hpp"
#include "app/RuntimeMovementIntentInputStep.hpp"
#include "app/RuntimeTargetInteractionInputStep.hpp"
#include "app/RuntimeTargetInputRouter.hpp"
#include "commands/MovementCommandSource.hpp"
#include "focus/InputFocus.hpp"
#include "player/Player.hpp"
#include "player/PlayerActionGate.hpp"
#include "session/SessionCommandSource.hpp"
#include "simulation/SimulationWorld.hpp"
#include "targeting/TargetResolver.hpp"

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

void TestRuntimeInputRouterMapsMouseClickToMovementCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 96, 64 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(result.handled, "runtime input router should handle gameplay mouse click");
	Expect(result.queuedMovementCommand, "runtime input router should queue movement for mouse click");
	Expect(!result.queuedSessionCommand, "runtime input router should not queue session command for mouse click");
	Expect(movementCommands.size() == 1, "runtime input router should enqueue one movement command");
	std::vector<dev::MovementCommand> drained = movementCommands.drain();
	Expect(drained.size() == 1 && drained[0].type == dev::MovementCommandType::WalkTo, "runtime input router should map mouse click to WalkTo");
	Expect(drained.size() == 1 && drained[0].destination == dev::Point { 3, 2 }, "runtime input router should map screen position through tile map");
	Expect(sessionCommands.empty(), "runtime input router should leave session queue empty for movement input");
}

void TestRuntimeMovementInputRouterMapsMouseClickToMovementCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeMovementInputRouter router { movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 96, 64 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime movement input router should handle gameplay mouse click");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::WalkTo, "runtime movement input router should map click to WalkTo");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 3, 2 }, "runtime movement input router should map click through tile map");
}

void TestRuntimeMovementIntentInputStepMapsPointerIntentToCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	const dev::Player &player = world.players[0];
	dev::InputFocus focus { dev::FocusState { .owner = dev::InputOwner::Gameplay } };
	dev::PlayerActionGate gate {
		focus,
		dev::PlayerActionContext {},
	};
	dev::QueuedMovementCommandSource movementCommands;

	dev::RuntimeInputRouteResult result = dev::RuntimeMovementIntentInputStep { movementCommands }.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 96, 64 },
	        .pressed = true,
	    },
	    world.map,
	    0,
	    player,
	    focus,
	    gate);

	Expect(result.handled && result.queuedMovementCommand, "runtime movement intent input step should queue movement commands for movement intents");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::WalkTo, "runtime movement intent input step should build WalkTo commands");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 3, 2 }, "runtime movement intent input step should map pointer input through the tile map");
}

void TestRuntimeMovementInputRouterMapsTouchTapToMovementCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeMovementInputRouter router { movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::TouchTap,
	        .screenPosition = { 128, 32 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime movement input router should handle gameplay touch tap");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::WalkTo, "runtime movement input router should map touch tap to WalkTo");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 4, 1 }, "runtime movement input router should map touch tap through tile map");
}

void TestRuntimeInputFocusResolverMapsSessionModesToFocus()
{
	dev::RuntimeInputFocusResolver resolver;

	dev::FocusState gameplay = resolver.resolve(
	    dev::FocusState { .owner = dev::InputOwner::Gameplay },
	    dev::GameSessionMode::Gameplay);
	dev::FocusState paused = resolver.resolve(
	    dev::FocusState { .owner = dev::InputOwner::Gameplay },
	    dev::GameSessionMode::Paused);
	dev::FocusState inventory = resolver.resolve(
	    dev::FocusState { .owner = dev::InputOwner::Gameplay, .textEntryActive = true },
	    dev::GameSessionMode::Inventory);
	dev::FocusState empty = resolver.resolve(
	    dev::FocusState { .owner = dev::InputOwner::Dialogue },
	    dev::GameSessionMode::Empty);

	Expect(gameplay.owner == dev::InputOwner::Gameplay, "runtime input focus resolver should preserve gameplay focus in gameplay mode");
	Expect(paused.owner == dev::InputOwner::Menu, "runtime input focus resolver should route paused mode to menu focus");
	Expect(inventory.owner == dev::InputOwner::Inventory, "runtime input focus resolver should route inventory mode to inventory focus");
	Expect(inventory.textEntryActive, "runtime input focus resolver should preserve text entry state");
	Expect(empty.owner == dev::InputOwner::Dialogue, "runtime input focus resolver should preserve explicit focus in empty mode");
}

void TestRuntimeMovementInputContextBuilderSelectsPlayerAndBlockReason()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players.push_back(MakePlayer({ 3, 4 }));
	world.players[1].moveState = dev::PlayerMoveState::Stunned;

	std::optional<dev::RuntimeMovementInputContext> context = dev::RuntimeMovementInputContextBuilder {}.build(
	    dev::RuntimeInputContext {
	        .world = &world,
	        .playerId = 1,
	        .focusState = dev::FocusState { .owner = dev::InputOwner::Gameplay },
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });
	std::optional<dev::RuntimeMovementInputContext> inventoryContext = dev::RuntimeMovementInputContextBuilder {}.build(
	    dev::RuntimeInputContext {
	        .world = &world,
	        .playerId = 0,
	        .focusState = dev::FocusState { .owner = dev::InputOwner::Gameplay },
	        .sessionMode = dev::GameSessionMode::Inventory,
	    });
	std::optional<dev::RuntimeMovementInputContext> missingWorld = dev::RuntimeMovementInputContextBuilder {}.build(
	    dev::RuntimeInputContext {});
	std::optional<dev::RuntimeMovementInputContext> missingPlayer = dev::RuntimeMovementInputContextBuilder {}.build(
	    dev::RuntimeInputContext {
	        .world = &world,
	        .playerId = 2,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(context.has_value(), "runtime movement input context builder should build context for active players");
	Expect(context.has_value() && context->player == &world.players[1], "runtime movement input context builder should select requested player");
	Expect(context.has_value() && context->focusState.owner == dev::InputOwner::Gameplay, "runtime movement input context builder should preserve gameplay focus in gameplay mode");
	Expect(context.has_value() && context->blockReason == dev::PlayerActionBlockReason::Stunned, "runtime movement input context builder should compute player movement block reason");
	Expect(inventoryContext.has_value() && inventoryContext->focusState.owner == dev::InputOwner::Inventory, "runtime movement input context builder should apply session focus rules");
	Expect(inventoryContext.has_value() && inventoryContext->blockReason == dev::PlayerActionBlockReason::Focus, "runtime movement input context builder should expose focus block reason");
	Expect(!missingWorld.has_value(), "runtime movement input context builder should reject missing worlds");
	Expect(!missingPlayer.has_value(), "runtime movement input context builder should reject missing players");
}

void TestRuntimeBlockedPointerInputStepReportsBlockedPointerMovement()
{
	dev::RuntimeInputRouteResult blocked = dev::RuntimeBlockedPointerInputStep {}.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 32, 0 },
	        .pressed = true,
	    },
	    dev::PlayerActionBlockReason::Focus);
	dev::RuntimeInputRouteResult unblocked = dev::RuntimeBlockedPointerInputStep {}.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 32, 0 },
	        .pressed = true,
	    },
	    dev::PlayerActionBlockReason::None);
	dev::RuntimeInputRouteResult nonPointer = dev::RuntimeBlockedPointerInputStep {}.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'A',
	        .pressed = true,
	    },
	    dev::PlayerActionBlockReason::Focus);

	Expect(!blocked.handled, "runtime blocked pointer input step should report blocks without handling input");
	Expect(blocked.movementBlockReason == std::optional<dev::PlayerActionBlockReason> { dev::PlayerActionBlockReason::Focus }, "runtime blocked pointer input step should preserve block reason");
	Expect(!unblocked.handled && !unblocked.movementBlockReason.has_value(), "runtime blocked pointer input step should ignore unblocked pointer input");
	Expect(!nonPointer.handled && !nonPointer.movementBlockReason.has_value(), "runtime blocked pointer input step should ignore non-pointer input");
}

void TestRuntimeInputRouterBlocksMovementWhenFocusDoesNotOwnGameplay()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult inventoryResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 32, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .focusState = dev::FocusState { .owner = dev::InputOwner::Inventory },
	        .sessionMode = dev::GameSessionMode::Inventory,
	    });

	dev::RuntimeInputRouteResult textEntryResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 64, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .focusState = dev::FocusState { .owner = dev::InputOwner::Gameplay, .textEntryActive = true },
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(!inventoryResult.handled, "runtime input router should ignore movement while inventory owns focus");
	Expect(!textEntryResult.handled, "runtime input router should ignore movement while text entry is active");
	Expect(inventoryResult.movementBlockReason == std::optional<dev::PlayerActionBlockReason> { dev::PlayerActionBlockReason::Focus }, "runtime input router should explain inventory movement block");
	Expect(textEntryResult.movementBlockReason == std::optional<dev::PlayerActionBlockReason> { dev::PlayerActionBlockReason::Focus }, "runtime input router should explain text-entry movement block");
	Expect(movementCommands.empty(), "runtime input router should not queue blocked movement input");
	Expect(sessionCommands.empty(), "runtime input router should not convert blocked movement into session commands");
}

void TestRuntimeInputRouterMapsTargetClickToMoveThenAct()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	FixedTargetResolver targets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 42,
		    .tile = { 0, 0 },
		}
	};

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 64, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	        .targetResolver = &targets,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime input router should route target-aware click");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::MoveThenAct, "enemy click should become MoveThenAct");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 2, 0 }, "target-aware click should use clicked tile");
	Expect(commands.size() == 1 && commands[0].destinationAction.has_value(), "enemy click should carry destination action");
	Expect(commands.size() == 1 && commands[0].destinationAction->type == dev::DestinationActionType::Attack, "enemy click should carry attack action");
	Expect(commands.size() == 1 && commands[0].destinationAction->target.id == 42, "enemy click should preserve target id");
	Expect(sessionCommands.empty(), "target-aware click should not queue session commands");
}

void TestRuntimeTargetInteractionInputStepBuildsInteractionCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	FixedTargetResolver targets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 47,
		    .tile = { 0, 0 },
		}
	};
	dev::FocusState focusState;
	dev::InputFocus focus { focusState };
	dev::PlayerActionContext context;
	dev::PlayerActionGate gate { focus, context };
	dev::QueuedMovementCommandSource movementCommands;

	dev::RuntimeInputRouteResult result = dev::RuntimeTargetInteractionInputStep { movementCommands }.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 64, 0 },
	        .pressed = true,
	    },
	    world.map,
	    targets,
	    0,
	    world.players[0],
	    gate);

	Expect(result.handled && result.queuedMovementCommand, "runtime target interaction input step should queue target-aware movement commands");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::MoveThenAct, "runtime target interaction input step should map enemy targets to MoveThenAct");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 2, 0 }, "runtime target interaction input step should resolve screen position through the tile map");
	Expect(commands.size() == 1 && commands[0].destinationAction.has_value(), "runtime target interaction input step should attach destination action");
	Expect(commands.size() == 1 && commands[0].destinationAction->type == dev::DestinationActionType::Attack, "runtime target interaction input step should preserve attack intent");
	Expect(commands.size() == 1 && commands[0].destinationAction->target.id == 47, "runtime target interaction input step should preserve target identity");
}

void TestRuntimeTargetInputRouterMapsTargetClickToMoveThenAct()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	FixedTargetResolver targets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 47,
		    .tile = { 0, 0 },
		}
	};
	dev::FocusState focusState;
	dev::InputFocus focus { focusState };
	dev::PlayerActionContext context;
	dev::PlayerActionGate gate { focus, context };
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeTargetInputRouter router { movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 64, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	        .targetResolver = &targets,
	    },
	    world.players[0],
	    gate);

	Expect(result.handled && result.queuedMovementCommand, "runtime target input router should route target-aware click");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::MoveThenAct, "runtime target input router should map enemy click to MoveThenAct");
	Expect(commands.size() == 1 && commands[0].destinationAction.has_value(), "runtime target input router should attach destination action");
	Expect(commands.size() == 1 && commands[0].destinationAction->target.id == 47, "runtime target input router should preserve target identity");
}

void TestRuntimeInputRouterMapsStandGroundTargetClickToStandAndAct()
{
	dev::SimulationWorld world;
	dev::Player player = MakePlayer({ 0, 0 });
	player.movementModifiers.standGround = true;
	world.players.push_back(player);
	FixedTargetResolver targets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 77,
		    .tile = { 0, 0 },
		}
	};

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 32, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	        .targetResolver = &targets,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime input router should route stand-ground target click");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::StandAndAct, "stand-ground enemy click should become StandAndAct");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 1, 0 }, "stand-ground target click should preserve clicked tile");
	Expect(commands.size() == 1 && commands[0].destinationAction->type == dev::DestinationActionType::Attack, "stand-ground target click should carry attack action");
}

} // namespace

int main()
{
	TestRuntimeInputRouterMapsMouseClickToMovementCommand();
	TestRuntimeMovementInputRouterMapsMouseClickToMovementCommand();
	TestRuntimeMovementIntentInputStepMapsPointerIntentToCommand();
	TestRuntimeMovementInputRouterMapsTouchTapToMovementCommand();
	TestRuntimeInputFocusResolverMapsSessionModesToFocus();
	TestRuntimeMovementInputContextBuilderSelectsPlayerAndBlockReason();
	TestRuntimeBlockedPointerInputStepReportsBlockedPointerMovement();
	TestRuntimeInputRouterBlocksMovementWhenFocusDoesNotOwnGameplay();
	TestRuntimeInputRouterMapsTargetClickToMoveThenAct();
	TestRuntimeTargetInteractionInputStepBuildsInteractionCommand();
	TestRuntimeTargetInputRouterMapsTargetClickToMoveThenAct();
	TestRuntimeInputRouterMapsStandGroundTargetClickToStandAndAct();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
