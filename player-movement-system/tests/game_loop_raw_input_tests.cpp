#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "app/GameLoop.hpp"
#include "input/RawInputSource.hpp"
#include "session/SessionCommandLog.hpp"
#include "session/SessionCommandLogFileStore.hpp"
#include "simulation/WorldEntityService.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestGameLoopRoutesRawInputHotkeysThroughSessionCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_raw_session_input_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "raw session input test should create startup script");

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .rawInputSources = { &rawInput } },
		    .input = { .bindings = dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'S' } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw hotkey input");
	Expect(result.summary.sessionCommandResults.size() == 1, "game loop should dispatch routed session command");
	Expect(result.summary.sessionCommandResults.size() == 1 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "routed pause hotkey should apply");
	Expect(result.finalMode == dev::GameSessionMode::Paused, "routed pause hotkey should update final mode");
	Expect(rawInput.empty(), "game loop should drain raw input source once");
	Expect(loop.sessionEvents().events().size() == 2, "startup script and routed hotkey should share session event sink");

	std::filesystem::remove_all(root);
}

void TestGameLoopRoutesRawMouseInputThroughMovementCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_raw_movement_input_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "raw movement input test should create startup script");

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	bool sawWalkCommand = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::WalkTo)
			sawWalkCommand = true;
	}

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw mouse input");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue movement command from routed raw input");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "routed raw mouse input should move player through simulation");
	Expect(sawWalkCommand, "routed raw mouse input should still emit movement command events");
	Expect(rawInput.empty(), "game loop should drain raw mouse input source once");

	std::filesystem::remove_all(root);
}

void TestGameLoopUsesWorldTargetRegistryForRawMouseInput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_world_target_input_test";
	std::filesystem::remove_all(root);

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().targets.add({ .type = dev::TargetType::Enemy, .id = 301, .tile = { 1, 0 } });

	dev::GameLoopResult result = loop.runForResult();

	bool sawMoveThenAct = false;
	bool sawActionRejected = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::MoveThenAct)
			sawMoveThenAct = true;
		if (event.type == dev::MovementEventType::ActionRejected)
			sawActionRejected = true;
	}

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw target-aware mouse input through world registry");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue world-target movement command");
	Expect(sawMoveThenAct, "world target registry click should become MoveThenAct command");
	Expect(sawActionRejected, "unregistered combat target should still reject at action layer");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "world target registry click should move player to target tile");

	std::filesystem::remove_all(root);
}

void TestGameLoopUsesWorldItemTargetForRawPickupInput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_world_item_input_test";
	std::filesystem::remove_all(root);

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::WorldEntityService {}.spawnItem(loop.session().world(), {
	    .id = 701,
	    .tile = { 1, 0 },
	});

	dev::GameLoopResult result = loop.runForResult();

	bool sawMoveThenAct = false;
	bool sawPickupExecuted = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::MoveThenAct)
			sawMoveThenAct = true;
		if (event.type == dev::MovementEventType::ActionExecuted && event.actionType == dev::DestinationActionType::Pickup)
			sawPickupExecuted = true;
	}

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw item mouse input through world registry");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue world-item movement command");
	Expect(sawMoveThenAct, "world item target click should become MoveThenAct command");
	Expect(sawPickupExecuted, "world item target click should execute pickup action");
	Expect(loop.session().world().items.empty(), "pickup action should remove item from world");
	Expect(loop.session().world().targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "pickup action should remove item target");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].inventory.items.size() == 1, "pickup action should transfer item to player inventory");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].inventory.items[0].id == 701, "pickup action should preserve transferred item identity");

	std::filesystem::remove_all(root);
}

void TestGameLoopLeavesItemWhenInventoryFull()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_full_inventory_pickup_test";
	std::filesystem::remove_all(root);

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.capacity = 1;
	loop.session().world().players[0].inventory.items.push_back({ .id = 800, .tile = { 0, 0 } });
	dev::WorldEntityService {}.spawnItem(loop.session().world(), {
	    .id = 801,
	    .tile = { 1, 0 },
	});

	dev::GameLoopResult result = loop.runForResult();

	bool sawPickupExecuted = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::ActionExecuted && event.actionType == dev::DestinationActionType::Pickup)
			sawPickupExecuted = true;
	}

	Expect(sawPickupExecuted, "full inventory pickup should still execute pickup action");
	Expect(loop.session().world().items.size() == 1 && loop.session().world().items[0].id == 801, "full inventory pickup should leave item in world");
	Expect(loop.session().world().targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::Item, "full inventory pickup should leave item target");
	Expect(loop.session().world().players[0].inventory.items.size() == 1 && loop.session().world().players[0].inventory.items[0].id == 800, "full inventory pickup should not alter inventory");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotRouteBlockedRawMovementInput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_raw_blocked_input_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "blocked raw input test should create startup script");

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .rawInputSources = { &rawInput } },
		    .input = { .focusState = dev::FocusState { .owner = dev::InputOwner::Gameplay, .textEntryActive = true } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.rawInputEventsRouted == 0, "game loop should not count blocked raw movement as routed");
	Expect(result.summary.movementInputBlockReasons.size() == 1 && result.summary.movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "game loop should summarize blocked raw movement reason");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].movementInputBlockReasons.size() == 1 && result.frameReports[0].movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "game loop frame report should preserve blocked raw movement reason");
	Expect(result.summary.movementCommandsQueued == 0, "game loop should not queue blocked raw movement");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 0, 0 }, "blocked raw movement should not move player");
	Expect(rawInput.empty(), "game loop should still drain inspected raw input");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestGameLoopRoutesRawInputHotkeysThroughSessionCommands();
	TestGameLoopRoutesRawMouseInputThroughMovementCommands();
	TestGameLoopUsesWorldTargetRegistryForRawMouseInput();
	TestGameLoopUsesWorldItemTargetForRawPickupInput();
	TestGameLoopLeavesItemWhenInventoryFull();
	TestGameLoopDoesNotRouteBlockedRawMovementInput();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
