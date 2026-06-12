#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/GameLoop.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"
#include "app/RuntimeTraceService.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
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

bool ContainsLineFragment(const std::vector<std::string> &lines, std::string_view fragment)
{
	for (const std::string &line : lines) {
		if (line.find(fragment) != std::string::npos)
			return true;
	}
	return false;
}

void TestRuntimeFrameTraceFileStoreSavesAndLoadsLines()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_file_store_test";
	const std::filesystem::path path = root / "frame.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	std::vector<std::string> lines {
		"frame rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=1 inventoryResults=2 movementScripts=0 movementQueued=1",
		"inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=955 slot=Weapon",
		"movementEvent[0] type=CommandAccepted player=0 tile=(0,0) command=WalkTo",
	};

	dev::RuntimeFrameTraceFileStore store;
	Expect(store.save(path, lines), "runtime frame trace file store should save lines");
	std::optional<std::vector<std::string>> loaded = store.load(path);

	Expect(loaded.has_value(), "runtime frame trace file store should load saved lines");
	Expect(loaded.has_value() && *loaded == lines, "runtime frame trace file store should preserve exact lines");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "runtime frame trace file store should remove temp file after save");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameTraceFileStoreRejectsMissingFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_missing_test";
	const std::filesystem::path path = root / "missing.trace";
	std::filesystem::remove_all(root);

	dev::RuntimeFrameTraceFileStore store;
	Expect(!store.load(path).has_value(), "runtime frame trace file store should reject missing file");

	std::filesystem::remove_all(root);
}

void TestRuntimeTraceServiceFormatsAndSavesRunTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_trace_service_test";
	const std::filesystem::path scriptPath = root / "trace_inventory.iicl";
	const std::filesystem::path tracePath = root / "run.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 956,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(scriptPath, log), "runtime trace service test should create inventory script");

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
	    .id = 956,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::GameLoopResult result = loop.runForResult();

	dev::RuntimeTraceService service;
	std::vector<std::string> lines = service.formatRun(result);
	Expect(service.saveRunTrace(tracePath, result), "runtime trace service should save full run trace");
	std::optional<std::vector<std::string>> loaded = dev::RuntimeFrameTraceFileStore {}.load(tracePath);

	Expect(!lines.empty(), "runtime trace service should format run lines");
	Expect(!lines.empty() && lines[0] == "run frames=1 frameReports=1 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=1 inventoryResults=1 movementScripts=0 movementQueued=0", "runtime trace service should include run summary");
	Expect(ContainsLineFragment(lines, "frame[0]"), "runtime trace service should include frame header");
	Expect(ContainsLineFragment(lines, "inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=956 slot=Weapon"), "runtime trace service should include frame trace detail");
	Expect(loaded.has_value() && *loaded == lines, "runtime trace service should persist exact formatted lines");

	std::filesystem::remove_all(root);
}

void TestRuntimeTraceServiceFormatsEmptyRun()
{
	dev::GameLoopResult result;
	std::vector<std::string> lines = dev::RuntimeTraceService {}.formatRun(result);

	Expect(lines.size() == 1, "runtime trace service should format empty run as summary only");
	Expect(lines.size() == 1 && lines[0] == "run frames=0 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime trace service should preserve empty run counts");
}

} // namespace

int main()
{
	TestRuntimeFrameTraceFileStoreSavesAndLoadsLines();
	TestRuntimeFrameTraceFileStoreRejectsMissingFile();
	TestRuntimeTraceServiceFormatsAndSavesRunTrace();
	TestRuntimeTraceServiceFormatsEmptyRun();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
