#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "app/GameLoop.hpp"
#include "app/RuntimeFrameTrace.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogFileStore.hpp"
#include "replay/MovementScriptSource.hpp"

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

void TestGameLoopDrainsRuntimeMovementScriptSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_movement_script_source_test";
	const std::filesystem::path scriptPath = root / "runtime_movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime movement script source test should create script file");

	dev::QueuedMovementScriptSource movementScripts;
	movementScripts.enqueue(scriptPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .movementScriptSources = { &movementScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeMovementScriptResults.size() == 1, "game loop should run runtime movement script source");
	Expect(result.summary.runtimeMovementScriptResults.size() == 1 && result.summary.runtimeMovementScriptResults[0].status == dev::MovementScriptRunStatus::Completed, "runtime movement script source should complete valid script");
	Expect(result.summary.runtimeMovementScriptResults.size() == 1 && result.summary.runtimeMovementScriptResults[0].replayReport.acceptedCount() == 1, "runtime movement script source should report accepted replay command");
	Expect(result.summary.movementCommandsQueued == 0, "runtime movement scripts should not inflate queued movement command count");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].movementScriptResults.size() == 1, "runtime frame report should include movement script results");
	std::vector<std::string> traceLines = result.frameReports.empty()
	    ? std::vector<std::string> {}
	    : dev::RuntimeFrameTrace {}.format(result.frameReports[0]);
	Expect(ContainsLineFragment(traceLines, "movementScripts=1"), "runtime frame trace should include movement script count");
	Expect(ContainsLineFragment(traceLines, "movementScript[0] status=Completed results=1 accepted=1 rejected=0"), "runtime frame trace should include movement script replay detail");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "runtime movement script source should move player through simulation");
	Expect(movementScripts.empty(), "game loop should drain runtime movement script source");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotDrainMovementScriptSourcesWithoutActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_no_world_movement_script_source_test";
	const std::filesystem::path scriptPath = root / "movement.imcl";
	std::filesystem::remove_all(root);

	dev::QueuedMovementScriptSource movementScripts;
	movementScripts.enqueue(scriptPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .movementScriptSources = { &movementScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeMovementScriptResults.empty(), "game loop should not run movement script sources without an active world");
	Expect(movementScripts.size() == 1, "game loop should preserve movement script paths until a world exists");

	std::filesystem::remove_all(root);
}

void TestGameLoopDrainsRuntimeInventoryScriptSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_source_test";
	const std::filesystem::path scriptPath = root / "runtime_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 953,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime inventory script source test should create script file");

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
	    .id = 953,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "game loop should run runtime inventory script source");
	Expect(result.summary.runtimeInventoryScriptResults.size() == 1 && result.summary.runtimeInventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script source should complete valid script");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script source should merge command results");
	Expect(loop.session().world().players[0].inventory.items.empty(), "runtime inventory script source should remove equipped item from bag");
	Expect(loop.session().world().players[0].inventory.equipment.weapon.has_value() && loop.session().world().players[0].inventory.equipment.weapon->id == 953, "runtime inventory script source should equip item");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Equipped, "runtime inventory script source should emit inventory event");
	Expect(inventoryScripts.empty(), "game loop should drain runtime inventory script source");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsRuntimeInventoryScriptLoadFailureWithoutStoppingFrames()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_source_load_failure_test";
	const std::filesystem::path missingPath = root / "missing.iicl";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(missingPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryScriptSources = { &inventoryScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "game loop should report runtime inventory script source result");
	Expect(result.summary.runtimeInventoryScriptResults.size() == 1 && result.summary.runtimeInventoryScriptResults[0].status == dev::InventoryScriptRunStatus::LoadFailed, "runtime inventory script source should report load failure");
	Expect(result.summary.inventoryCommandResults.empty(), "failed runtime inventory script source should not dispatch commands");
	Expect(result.summary.framesRun == 1, "runtime inventory script load failure should not stop frame updates");
	Expect(inventoryScripts.empty(), "failed runtime inventory script source should still drain after attempted run");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotDrainInventoryScriptSourcesWithoutActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_no_world_inventory_script_source_test";
	const std::filesystem::path scriptPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryScriptSources = { &inventoryScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeInventoryScriptResults.empty(), "game loop should not run inventory script sources without an active world");
	Expect(inventoryScripts.size() == 1, "game loop should preserve inventory script paths until a world exists");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestGameLoopDrainsRuntimeMovementScriptSources();
	TestGameLoopDoesNotDrainMovementScriptSourcesWithoutActiveWorld();
	TestGameLoopDrainsRuntimeInventoryScriptSources();
	TestGameLoopReportsRuntimeInventoryScriptLoadFailureWithoutStoppingFrames();
	TestGameLoopDoesNotDrainInventoryScriptSourcesWithoutActiveWorld();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
