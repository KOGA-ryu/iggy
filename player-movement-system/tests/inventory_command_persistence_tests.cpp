#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "inventory/EquipmentService.hpp"
#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogChecksum.hpp"
#include "inventory/InventoryCommandLogCodec.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandLogFrameCodec.hpp"
#include "inventory/InventoryCommandPacketByteCodec.hpp"
#include "inventory/InventoryCommandPacketListCodec.hpp"
#include "inventory/InventoryCommandReplayer.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "player/PlayerMovement.hpp"

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

void TestInventoryCommandLogReplaysThroughDispatcher()
{
	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 962,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryCommandReplayer replayer { dispatcher };
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 962,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	std::vector<dev::InventoryCommandResult> results = replayer.replay(log);

	Expect(results.size() == 2, "inventory command log should replay every command");
	Expect(results.size() == 2 && results[0].type == dev::InventoryCommandResultType::Applied, "inventory command replay should apply equip command");
	Expect(results.size() == 2 && results[1].type == dev::InventoryCommandResultType::Applied, "inventory command replay should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "inventory command replay should leave weapon slot empty after unequip");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 962, "inventory command replay should return unequipped item to bag");
	Expect(events.events().size() == 2, "inventory command replay should emit inventory events");
	Expect(events.events().size() == 2 && events.events()[0].type == dev::InventoryEventType::Equipped, "inventory command replay should emit equipped event");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::InventoryEventType::Unequipped, "inventory command replay should emit unequipped event");
}

void TestInventoryCommandLogCodecRoundTripsAndReplays()
{
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 963,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogCodec codec;
	dev::InventoryCommandLogBytes bytes = codec.encode(log);
	std::optional<dev::InventoryCommandLog> decoded = codec.decode(bytes);
	Expect(decoded.has_value(), "inventory command log codec should decode its own bytes");
	Expect(decoded.has_value() && decoded->commands().size() == 2, "inventory command log codec should preserve command count");
	Expect(decoded.has_value() && decoded->commands()[0].itemId == std::optional<dev::TargetId> { 963 }, "inventory command log codec should preserve equip item id");
	Expect(decoded.has_value() && decoded->commands()[1].slot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Weapon }, "inventory command log codec should preserve unequip slot");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 963,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryCommandDispatcher dispatcher { player };
	dev::InventoryCommandReplayer replayer { dispatcher };
	std::vector<dev::InventoryCommandResult> results = decoded.has_value()
	    ? replayer.replay(*decoded)
	    : std::vector<dev::InventoryCommandResult> {};

	Expect(results.size() == 2, "decoded inventory command log should replay");
	Expect(!player.inventory.equipment.weapon.has_value(), "decoded inventory command log should reproduce inventory state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 963, "decoded inventory command log should preserve item ownership");
}

void TestInventoryCommandLogCodecRejectsInvalidBytes()
{
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 964,
	});

	dev::InventoryCommandLogCodec codec;
	dev::InventoryCommandLogBytes bytes = codec.encode(log);

	dev::InventoryCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "inventory command log codec should reject bad magic");

	dev::InventoryCommandLogBytes badVersion = bytes;
	badVersion[4] = 2;
	Expect(!codec.decode(badVersion).has_value(), "inventory command log codec should reject bad version");

	dev::InventoryCommandLogBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "inventory command log codec should reject truncated bytes");

	dev::InventoryCommandLogBytes corrupted = bytes;
	corrupted[12] ^= 0x01U;
	Expect(!codec.decode(corrupted).has_value(), "inventory command log codec should reject checksum mismatch");
}

void TestInventoryCommandLogChecksumValidatesTrailingChecksum()
{
	dev::InventoryCommandLogBytes bytes { 1, 2, 3, 4 };
	dev::InventoryCommandLogChecksum checksum;
	const uint32_t expected = checksum.compute(bytes, bytes.size());

	checksum.appendTo(bytes);

	Expect(bytes.size() == 8, "inventory command log checksum should append four checksum bytes");
	Expect(checksum.hasValidTrailingChecksum(bytes, 4), "inventory command log checksum should validate appended checksum");
	Expect(expected == checksum.compute(bytes, 4), "inventory command log checksum should compute payload hash only");

	bytes[0] ^= 0xFFU;
	Expect(!checksum.hasValidTrailingChecksum(bytes, 4), "inventory command log checksum should reject mutated payload");
}

void TestInventoryCommandPacketListCodecFramesPacketBytes()
{
	dev::InventoryCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		.hasItemId = 1,
		.itemId = 100,
	};
	dev::InventoryCommandPacketByteCodec packetCodec;
	dev::InventoryCommandBytes packetBytes = packetCodec.encode(packet);

	dev::InventoryCommandLogBytes bytes = dev::InventoryCommandPacketListCodec {}.encode({ packetBytes, packetBytes });
	std::optional<std::vector<dev::InventoryCommandBytes>> decoded = dev::InventoryCommandPacketListCodec {}.decode(bytes);

	Expect(decoded.has_value(), "inventory command packet list codec should decode encoded packet lists");
	Expect(decoded.has_value() && decoded->size() == 2, "inventory command packet list codec should preserve packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packetBytes, "inventory command packet list codec should preserve first packet bytes");
	Expect(decoded.has_value() && (*decoded)[1] == packetBytes, "inventory command packet list codec should preserve second packet bytes");
}

void TestInventoryCommandPacketListCodecRejectsInvalidSizes()
{
	dev::InventoryCommandLogBytes missingCount { 1, 2 };
	dev::InventoryCommandLogBytes wrongSize {
		1, 0, 0, 0,
		1, 2, 3,
	};

	dev::InventoryCommandPacketListCodec codec;
	Expect(!codec.decode(missingCount).has_value(), "inventory command packet list codec should reject missing command count");
	Expect(!codec.decode(wrongSize).has_value(), "inventory command packet list codec should reject packet lists with invalid size");
}

void TestInventoryCommandLogFrameCodecFramesPacketBytes()
{
	dev::InventoryCommandPacketByteCodec packetCodec;
	std::vector<dev::InventoryCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		    .hasItemId = 1,
		    .itemId = 962,
		}),
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
		    .hasSlot = 1,
		    .slot = static_cast<uint8_t>(dev::EquipmentSlot::Weapon),
		}),
	};

	dev::InventoryCommandLogFrameCodec frameCodec;
	dev::InventoryCommandLogBytes bytes = frameCodec.encode(packets);
	std::optional<std::vector<dev::InventoryCommandBytes>> decoded = frameCodec.decode(bytes);

	Expect(bytes.size() == 48, "inventory command log frame codec should write header, packets, and checksum");
	Expect(bytes.size() == 48 && bytes[0] == 'I' && bytes[1] == 'I' && bytes[2] == 'C' && bytes[3] == 'L', "inventory command log frame codec should write magic");
	Expect(bytes.size() == 48 && bytes[4] == 1 && bytes[8] == 2, "inventory command log frame codec should write version and command count");
	Expect(decoded.has_value() && decoded->size() == 2, "inventory command log frame codec should restore packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packets[0], "inventory command log frame codec should preserve first packet");
	Expect(decoded.has_value() && (*decoded)[1] == packets[1], "inventory command log frame codec should preserve second packet");
}

void TestInventoryCommandLogFrameCodecRejectsInvalidFrames()
{
	dev::InventoryCommandPacketByteCodec packetCodec;
	std::vector<dev::InventoryCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		    .hasItemId = 1,
		    .itemId = 962,
		}),
	};
	dev::InventoryCommandLogFrameCodec frameCodec;
	dev::InventoryCommandLogBytes bytes = frameCodec.encode(packets);

	dev::InventoryCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!frameCodec.decode(badMagic).has_value(), "inventory command log frame codec should reject checksum-protected bad magic");

	dev::InventoryCommandLogBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 2;
	dev::InventoryCommandLogChecksum {}.appendTo(badVersion);
	Expect(!frameCodec.decode(badVersion).has_value(), "inventory command log frame codec should reject unsupported version");

	dev::InventoryCommandLogBytes wrongCount = bytes;
	wrongCount.resize(wrongCount.size() - 4U);
	wrongCount[8] = 2;
	dev::InventoryCommandLogChecksum {}.appendTo(wrongCount);
	Expect(!frameCodec.decode(wrongCount).has_value(), "inventory command log frame codec should reject payload size mismatch");
}

void TestInventoryCommandLogFileStoreSavesLoadsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_log_file_store_replay_test";
	const std::filesystem::path path = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 965,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory command log file store should save log");
	std::optional<dev::InventoryCommandLog> loaded = store.load(path);

	Expect(loaded.has_value(), "inventory command log file store should load saved log");
	Expect(loaded.has_value() && loaded->commands().size() == 2, "inventory command log file store should preserve command count");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "inventory command log file store should remove temp file after save");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 965,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryCommandReplayer replayer { dispatcher };
	std::vector<dev::InventoryCommandResult> results = loaded.has_value()
	    ? replayer.replay(*loaded)
	    : std::vector<dev::InventoryCommandResult> {};

	Expect(results.size() == 2, "loaded inventory command log should replay");
	Expect(!player.inventory.equipment.weapon.has_value(), "loaded inventory command log should reproduce inventory state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 965, "loaded inventory command log should preserve item ownership");
	Expect(events.events().size() == 2, "loaded inventory command log replay should emit events");

	std::filesystem::remove_all(root);
}

void TestInventoryCommandLogFileStoreRejectsCorruptAndMissingFiles()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_log_file_store_corrupt_test";
	const std::filesystem::path missingPath = root / "missing.iicl";
	const std::filesystem::path corruptPath = root / "corrupt.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	{
		std::ofstream output { corruptPath, std::ios::binary | std::ios::trunc };
		output << "not an inventory command log";
	}

	dev::InventoryCommandLogFileStore store;
	Expect(!store.load(missingPath).has_value(), "inventory command log file store should reject missing file");
	Expect(!store.load(corruptPath).has_value(), "inventory command log file store should reject corrupt file");

	std::filesystem::remove_all(root);
}

void TestInventoryScriptRunnerRunsSavedInventoryScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_script_runner_test";
	const std::filesystem::path path = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 966,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory script runner test should create script file");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 966,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryScriptRunner runner { dispatcher };
	dev::InventoryScriptRunResult result = runner.run(path);

	Expect(result.status == dev::InventoryScriptRunStatus::Completed, "inventory script runner should complete valid script files");
	Expect(result.commandResults.size() == 2, "inventory script runner should return per-command results");
	Expect(result.commandResults.size() == 2 && result.commandResults[0].type == dev::InventoryCommandResultType::Applied, "inventory script runner should apply equip command");
	Expect(result.commandResults.size() == 2 && result.commandResults[1].type == dev::InventoryCommandResultType::Applied, "inventory script runner should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "inventory script runner should reproduce unequipped state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 966, "inventory script runner should preserve item ownership");
	Expect(events.events().size() == 2, "inventory script runner should still emit dispatcher events");

	std::filesystem::remove_all(root);
}

void TestInventoryScriptRunnerReportsLoadFailureAndCommandRejectionSeparately()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_script_runner_failure_test";
	const std::filesystem::path path = root / "inventory.iicl";
	const std::filesystem::path missingPath = root / "missing.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::Player player = MakePlayer();
	player.inventory.items.push_back({ .id = 967 });
	dev::InventoryCommandDispatcher dispatcher { player };
	dev::InventoryScriptRunner runner { dispatcher };

	dev::InventoryScriptRunResult missing = runner.run(missingPath);
	Expect(missing.status == dev::InventoryScriptRunStatus::LoadFailed, "inventory script runner should report missing file load failure");
	Expect(missing.commandResults.empty(), "missing inventory script should not dispatch commands");

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 967,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory script runner rejection test should create script file");

	dev::InventoryScriptRunResult rejected = runner.run(path);
	Expect(rejected.status == dev::InventoryScriptRunStatus::Completed, "inventory script runner should complete loadable scripts even when commands reject");
	Expect(rejected.commandResults.size() == 1, "inventory script runner should return rejected command result");
	Expect(rejected.commandResults.size() == 1 && rejected.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "inventory script runner should preserve command-level rejection");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 967, "rejected inventory script command should not mutate inventory");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestInventoryCommandLogReplaysThroughDispatcher();
	TestInventoryCommandLogCodecRoundTripsAndReplays();
	TestInventoryCommandLogCodecRejectsInvalidBytes();
	TestInventoryCommandLogChecksumValidatesTrailingChecksum();
	TestInventoryCommandPacketListCodecFramesPacketBytes();
	TestInventoryCommandPacketListCodecRejectsInvalidSizes();
	TestInventoryCommandLogFrameCodecFramesPacketBytes();
	TestInventoryCommandLogFrameCodecRejectsInvalidFrames();
	TestInventoryCommandLogFileStoreSavesLoadsAndReplays();
	TestInventoryCommandLogFileStoreRejectsCorruptAndMissingFiles();
	TestInventoryScriptRunnerRunsSavedInventoryScript();
	TestInventoryScriptRunnerReportsLoadFailureAndCommandRejectionSeparately();
	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "inventory_command_persistence_tests passed\n";
	return EXIT_SUCCESS;
}
