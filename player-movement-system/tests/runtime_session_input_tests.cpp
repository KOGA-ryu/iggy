#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeSessionInputRouter.hpp"
#include "app/RuntimeSessionModeTogglePolicy.hpp"
#include "commands/MovementCommandSource.hpp"
#include "session/SessionCommandSource.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
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

} // namespace

int main()
{
	TestRuntimeInputRouterMapsHotkeysToSessionCommands();
	TestRuntimeSessionInputRouterTogglesLifecycleModes();
	TestRuntimeSessionModeTogglePolicyMapsHotkeysToRequestedModes();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
