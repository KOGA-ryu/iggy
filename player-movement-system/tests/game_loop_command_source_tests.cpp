#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "app/GameLoop.hpp"
#include "commands/MovementCommandSource.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "session/SessionCommandLog.hpp"
#include "session/SessionCommandLogFileStore.hpp"
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

void TestGameLoopDrainsRuntimeSessionCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_runtime_command_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedSessionCommandSource source;
	source.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 9, 3 }, .playerHitPoints = 22 },
	});
	source.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .sessionCommandSources = { &source } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(!result.setup.startupScriptRan, "runtime command source test should not run startup script");
	Expect(result.summary.sessionCommandResults.size() == 2, "game loop should dispatch runtime session commands");
	Expect(result.summary.sessionCommandResults.size() == 2 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "game loop should apply runtime new-game command");
	Expect(result.summary.sessionCommandResults.size() == 2 && result.summary.sessionCommandResults[1].type == dev::SessionCommandResultType::Applied, "game loop should apply runtime mode command");
	Expect(result.summary.framesRun == 1, "game loop should still run frame after runtime commands");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 9, 3 }, "runtime command source should initialize session world");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].combatStats.hitPoints == 22, "runtime command source should preserve new-game settings");
	Expect(result.finalMode == dev::GameSessionMode::Inventory, "runtime command source should set final session mode");
	Expect(source.empty(), "game loop should drain runtime source commands once");
	Expect(loop.sessionEvents().events().size() == 2, "game loop should emit events for runtime source commands");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsStartupScriptBeforeRuntimeCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_then_source_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 8 }, .playerHitPoints = 12 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "game loop startup/source test should create startup script");

	dev::QueuedSessionCommandSource source;
	source.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .sessionCommandSources = { &source } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptResult.commandResults.size() == 1, "startup script should run before runtime sources");
	Expect(result.summary.sessionCommandResults.size() == 1, "runtime source should run after startup script");
	Expect(result.summary.sessionCommandResults.size() == 1 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "runtime mode command should apply after startup creates a world");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 8 }, "startup script should initialize world before runtime mode command");
	Expect(result.finalMode == dev::GameSessionMode::Paused, "runtime command should be able to change mode after startup");
	Expect(loop.sessionEvents().events().size() == 2, "startup and runtime commands should share the same session event sink");

	std::filesystem::remove_all(root);
}

void TestGameLoopDrainsRuntimeMovementCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_movement_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedSessionCommandSource sessionSource;
	sessionSource.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::QueuedMovementCommandSource movementSource;
	movementSource.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = {
		        .sessionCommandSources = { &sessionSource },
		        .movementCommandSources = { &movementSource },
		    },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	bool sawCommandAccepted = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::WalkTo)
			sawCommandAccepted = true;
	}

	Expect(result.summary.sessionCommandResults.size() == 1, "game loop movement source test should dispatch session startup first");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue runtime movement commands");
	Expect(result.summary.framesRun == 1, "game loop should run a frame after queueing movement");
	Expect(movementSource.empty(), "game loop should drain movement source commands once");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "movement command source should move player through simulation");
	Expect(sawCommandAccepted, "movement command source should still pass through movement command dispatcher events");

	std::filesystem::remove_all(root);
}

void TestGameLoopDrainsRuntimeInventoryCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 950,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryCommandSources = { &inventoryCommands } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 950,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.inventoryCommandResults.size() == 1, "game loop should dispatch runtime inventory command source");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Applied, "game loop should apply valid runtime inventory command");
	Expect(loop.session().world().players[0].inventory.items.empty(), "runtime inventory equip should remove item from bag");
	Expect(loop.session().world().players[0].inventory.equipment.weapon.has_value() && loop.session().world().players[0].inventory.equipment.weapon->id == 950, "runtime inventory equip should fill equipment slot");
	Expect(inventoryCommands.empty(), "game loop should drain runtime inventory command source");
	Expect(loop.inventoryEvents().events().size() == 1, "game loop should record runtime inventory event");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Equipped, "game loop inventory event should report equipped item");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].itemId == 950, "game loop inventory event should preserve item id");

	std::filesystem::remove_all(root);
}

void TestGameLoopEmitsRejectedInventoryEventForMissingPlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_missing_player_inventory_event_test";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 952,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryCommandSources = { &inventoryCommands } },
		    .input = { .playerId = 3 },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.inventoryCommandResults.size() == 1, "game loop should reject inventory command for missing player");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Rejected, "missing player inventory command should be rejected");
	Expect(loop.inventoryEvents().events().size() == 1, "missing player inventory command should emit rejected event");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Rejected, "missing player inventory event should be rejected");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].commandType == dev::InventoryCommandType::EquipItem, "missing player inventory event should preserve command type");
	Expect(inventoryCommands.empty(), "missing player inventory command should still drain after rejection");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotDrainInventorySourcesWithoutActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_no_world_inventory_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 951,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryCommandSources = { &inventoryCommands } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.inventoryCommandResults.empty(), "game loop should not dispatch inventory commands without an active world");
	Expect(inventoryCommands.size() == 1, "game loop should preserve inventory commands until a world exists");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotDrainMovementSourcesWithoutActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_movement_source_empty_test";
	std::filesystem::remove_all(root);

	dev::QueuedMovementCommandSource movementSource;
	movementSource.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .movementCommandSources = { &movementSource } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.movementCommandsQueued == 0, "game loop should not queue movement commands without an active world");
	Expect(result.summary.framesRun == 1, "game loop should still run bounded frames without an active world");
	Expect(movementSource.size() == 1, "movement source should keep commands when no active world can receive them");
	Expect(result.finalMode == dev::GameSessionMode::Empty, "movement-only loop should leave session empty");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestGameLoopDrainsRuntimeSessionCommandSources();
	TestGameLoopRunsStartupScriptBeforeRuntimeCommandSources();
	TestGameLoopDrainsRuntimeMovementCommandSources();
	TestGameLoopDrainsRuntimeInventoryCommandSources();
	TestGameLoopEmitsRejectedInventoryEventForMissingPlayer();
	TestGameLoopDoesNotDrainInventorySourcesWithoutActiveWorld();
	TestGameLoopDoesNotDrainMovementSourcesWithoutActiveWorld();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
