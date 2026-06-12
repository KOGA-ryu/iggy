#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "app/GameLoop.hpp"
#include "commands/MovementCommandSource.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryScriptSource.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestGameLoopBuildsRuntimeFrameReports()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_frame_report_test";
	const std::filesystem::path scriptPath = root / "frame_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 954,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "frame report test should create inventory script");

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);
	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::QueuedMovementCommandSource movementCommands;
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = {
		        .movementCommandSources = { &movementCommands },
		        .inventoryCommandSources = { &inventoryCommands },
		        .inventoryScriptSources = { &inventoryScripts },
		    },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.capacity = 2;
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 954,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.frameReports.size() == 1, "game loop should record one runtime frame report per frame");
	if (result.frameReports.empty()) {
		std::filesystem::remove_all(root);
		return;
	}

	const dev::RuntimeFrameReport &report = result.frameReports[0];
	Expect(report.inventoryScriptResults.size() == 1, "runtime frame report should include inventory script results");
	Expect(report.inventoryScriptResults.size() == 1 && report.inventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime frame report should preserve inventory script status");
	Expect(report.inventoryCommandResults.size() == 2, "runtime frame report should include script and direct inventory command results");
	Expect(report.inventoryCommandResults.size() == 2 && report.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime frame report should include applied script command result");
	Expect(report.inventoryCommandResults.size() == 2 && report.inventoryCommandResults[1].type == dev::InventoryCommandResultType::Applied, "runtime frame report should include applied direct inventory command result");
	Expect(report.movementCommandsQueued == 1, "runtime frame report should include movement command count");
	Expect(!report.frameEvents.movementEvents().empty(), "runtime frame report should include simulation frame events");
	Expect(report.inventoryEvents.size() == 2, "runtime frame report should include inventory event deltas");
	Expect(report.inventoryEvents.size() == 2 && report.inventoryEvents[0].type == dev::InventoryEventType::Equipped, "runtime frame report should include equipped event");
	Expect(report.inventoryEvents.size() == 2 && report.inventoryEvents[1].type == dev::InventoryEventType::Unequipped, "runtime frame report should include unequipped event");
	Expect(std::string_view { report.framePolicy.modeName } == "Gameplay", "runtime frame report should include frame policy mode");
	Expect(report.framePolicy.policy.acceptCommands && report.framePolicy.policy.updatePlayers && report.framePolicy.policy.updateEnemies, "runtime frame report should include frame policy gates");
	Expect(result.summary.inventoryCommandResults.size() == report.inventoryCommandResults.size(), "game loop aggregate inventory command results should match frame report results");
	Expect(result.summary.movementCommandsQueued == report.movementCommandsQueued, "game loop aggregate movement count should match frame report count");
	Expect(result.summary.lastFrameEvents.movementEvents().size() == report.frameEvents.movementEvents().size(), "last frame events should mirror final runtime frame report");
	Expect(loop.session().world().players[0].inventory.items.size() == 1 && loop.session().world().players[0].inventory.items[0].id == 954, "frame report scenario should replay script then direct unequip");
	Expect(loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "frame report scenario should still run movement update");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestGameLoopBuildsRuntimeFrameReports();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
