#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "app/RuntimeInputContextBuilder.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "app/RuntimeMovementInputRouter.hpp"
#include "app/RuntimeSessionInputRouter.hpp"
#include "app/RuntimeSessionModeTogglePolicy.hpp"
#include "app/RuntimeStopMovementInputStep.hpp"
#include "commands/MovementCommandSource.hpp"
#include "focus/InputFocus.hpp"
#include "player/Player.hpp"
#include "player/PlayerActionGate.hpp"
#include "session/GameSession.hpp"
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
	TestRuntimeInputSettingsDefaultsToPrimaryGameplayInput();
	TestRuntimeInputContextBuilderHandlesMissingWorld();
	TestRuntimeInputContextBuilderUsesWorldTargetsUnlessOverridden();
	TestRuntimeInputRouteResultBuilderNamesRouteOutcomes();
	TestRuntimeInputRouterMapsHotkeysToSessionCommands();
	TestRuntimeSessionInputRouterTogglesLifecycleModes();
	TestRuntimeSessionModeTogglePolicyMapsHotkeysToRequestedModes();
	TestRuntimeInputRouterMapsStopHotkeyToMovementCommand();
	TestRuntimeStopMovementInputStepMapsStopHotkeyToCommand();
	TestRuntimeMovementInputRouterReportsBlockedStopReason();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
