#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "app/RuntimeInventoryCommandIntake.hpp"
#include "app/RuntimeInventoryScriptBatchRunner.hpp"
#include "app/RuntimeInventoryScriptIntake.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "player/Player.hpp"

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

void TestQueuedInventoryCommandSourceDrainsCommandsOnce()
{
	dev::QueuedInventoryCommandSource source;
	source.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 940,
	});
	source.enqueue({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	Expect(source.size() == 2, "queued inventory command source should track queued command count");
	std::vector<dev::InventoryCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued inventory command source should drain queued commands");
	Expect(source.empty(), "queued inventory command source should be empty after drain");
	Expect(source.drain().empty(), "queued inventory command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].itemId == std::optional<dev::TargetId> { 940 }, "queued inventory command source should preserve equip payload");
	Expect(drained.size() == 2 && drained[1].slot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Weapon }, "queued inventory command source should preserve unequip payload");
}

void TestQueuedInventoryScriptSourceDrainsPathsOnce()
{
	dev::QueuedInventoryScriptSource source;
	const std::filesystem::path first = "equip.iicl";
	const std::filesystem::path second = "swap.iicl";
	source.enqueue(first);
	source.enqueue(second);

	Expect(source.size() == 2, "queued inventory script source should track queued path count");
	std::vector<std::filesystem::path> drained = source.drain();
	Expect(drained.size() == 2, "queued inventory script source should drain queued paths");
	Expect(source.empty(), "queued inventory script source should be empty after drain");
	Expect(source.drain().empty(), "queued inventory script source should not drain paths twice");
	Expect(drained.size() == 2 && drained[0] == first, "queued inventory script source should preserve first path");
	Expect(drained.size() == 2 && drained[1] == second, "queued inventory script source should preserve second path");
}

void TestRuntimeInventoryCommandIntakeDispatchesOrRejectsCommands()
{
	dev::Player player;
	player.inventory.items.push_back({
	    .id = 10,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;

	std::vector<dev::InventoryCommandResult> applied = dev::RuntimeInventoryCommandIntake {}.dispatch(
	    {
	        {
	            .type = dev::InventoryCommandType::EquipItem,
	            .itemId = 10,
	        },
	    },
	    &player,
	    &events);

	Expect(applied.size() == 1 && applied[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory command intake should dispatch commands for active player");
	Expect(player.inventory.items.empty(), "runtime inventory command intake should apply equipment changes");
	Expect(player.inventory.equipment.weapon.has_value() && player.inventory.equipment.weapon->id == 10, "runtime inventory command intake should equip item into slot");
	Expect(events.events().size() == 1 && events.events()[0].type == dev::InventoryEventType::Equipped, "runtime inventory command intake should emit applied inventory event");

	std::vector<dev::InventoryCommandResult> rejected = dev::RuntimeInventoryCommandIntake {}.dispatch(
	    {
	        {
	            .type = dev::InventoryCommandType::EquipItem,
	            .itemId = 11,
	        },
	    },
	    nullptr,
	    &events);

	Expect(rejected.size() == 1 && rejected[0].type == dev::InventoryCommandResultType::Rejected, "runtime inventory command intake should reject commands without active player");
	Expect(rejected.size() == 1 && rejected[0].command.itemId == std::optional<dev::TargetId> { 11 }, "runtime inventory command intake should preserve rejected command payload");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::InventoryEventType::Rejected, "runtime inventory command intake should emit rejected event without active player");
	Expect(events.events().size() == 2 && events.events()[1].commandType == dev::InventoryCommandType::EquipItem, "runtime inventory command intake should preserve rejected event command type");
}

void TestRuntimeInventoryScriptIntakeRunsScriptsAgainstActivePlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_inventory_script_intake_test";
	const std::filesystem::path scriptPath = root / "runtime_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 12,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime inventory script intake test should save script");

	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 12,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;

	const dev::InventoryScriptRunResult result = dev::RuntimeInventoryScriptIntake {}.run(scriptPath, &player, &events);
	const dev::InventoryScriptRunResult missingPlayer = dev::RuntimeInventoryScriptIntake {}.run(scriptPath, nullptr, &events);

	Expect(result.status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script intake should run scripts against active player");
	Expect(result.commandResults.size() == 2, "runtime inventory script intake should replay every inventory command");
	Expect(result.commandResults.size() == 2 && result.commandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script intake should apply equip command");
	Expect(result.commandResults.size() == 2 && result.commandResults[1].type == dev::InventoryCommandResultType::Applied, "runtime inventory script intake should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "runtime inventory script intake should let script dispatch affect player equipment");
	Expect(events.events().size() == 2, "runtime inventory script intake should emit inventory events through provided sink");
	Expect(missingPlayer.status == dev::InventoryScriptRunStatus::NoActivePlayer, "runtime inventory script intake should report missing active player");
	Expect(missingPlayer.commandResults.empty(), "runtime inventory script intake should not replay without active player");
	Expect(events.events().size() == 2, "runtime inventory script intake should not emit extra events without active player");

	std::filesystem::remove_all(root);
}

void TestRuntimeInventoryScriptBatchRunnerPreservesPathOrder()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_inventory_script_batch_runner_test";
	const std::filesystem::path firstPath = root / "first.iicl";
	const std::filesystem::path missingPath = root / "missing.iicl";
	const std::filesystem::path thirdPath = root / "third.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog firstLog;
	firstLog.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 13,
	});
	dev::InventoryCommandLog thirdLog;
	thirdLog.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(firstPath, firstLog), "runtime inventory script batch runner test should save first script");
	Expect(store.save(thirdPath, thirdLog), "runtime inventory script batch runner test should save third script");

	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 13,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;

	std::vector<dev::InventoryScriptRunResult> results = dev::RuntimeInventoryScriptBatchRunner {}.run(
	    { firstPath, missingPath, thirdPath },
	    &player,
	    &events);

	Expect(results.size() == 3, "runtime inventory script batch runner should produce one result per script path");
	Expect(results.size() == 3 && results[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script batch runner should keep first script result order");
	Expect(results.size() == 3 && results[1].status == dev::InventoryScriptRunStatus::LoadFailed, "runtime inventory script batch runner should keep missing script result order");
	Expect(results.size() == 3 && results[2].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script batch runner should continue after load failure");
	Expect(results.size() == 3 && results[0].commandResults.size() == 1 && results[0].commandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script batch runner should replay first script");
	Expect(results.size() == 3 && results[2].commandResults.size() == 1 && results[2].commandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script batch runner should replay later scripts");
	Expect(events.events().size() == 2, "runtime inventory script batch runner should emit events for completed script commands only");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestQueuedInventoryCommandSourceDrainsCommandsOnce();
	TestQueuedInventoryScriptSourceDrainsPathsOnce();
	TestRuntimeInventoryCommandIntakeDispatchesOrRejectsCommands();
	TestRuntimeInventoryScriptIntakeRunsScriptsAgainstActivePlayer();
	TestRuntimeInventoryScriptBatchRunnerPreservesPathOrder();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
