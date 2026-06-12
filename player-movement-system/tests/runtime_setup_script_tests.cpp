#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "app/GameLoop.hpp"
#include "app/RuntimeStartupScriptIntake.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "items/Item.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogFileStore.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandLog.hpp"
#include "session/SessionCommandLogFileStore.hpp"
#include "session/SessionEventRecorder.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
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

} // namespace

int main()
{
	TestRuntimeStartupScriptIntakeRunsLifecycleScript();
	TestGameLoopRunsStartupScriptAndFrames();
	TestGameLoopReportsStartupScriptLoadFailure();
	TestGameLoopRunsInventoryScriptAgainstActivePlayer();
	TestGameLoopRunsInventoryScriptAfterStartupScript();
	TestGameLoopRunsMovementScriptAfterStartupScript();
	TestGameLoopReportsInventoryScriptLoadFailure();
	TestGameLoopReportsInventoryScriptWithoutActivePlayer();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
