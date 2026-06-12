#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/GameLoop.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"

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

void TestGameLoopSavesConfiguredRunTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_run_trace_test";
	const std::filesystem::path tracePath = root / "run.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> loaded = dev::RuntimeFrameTraceFileStore {}.load(tracePath);

	Expect(result.output.runTraceSaveAttempted, "game loop should attempt configured run trace save");
	Expect(result.output.runTraceSaved, "game loop should report successful run trace save");
	Expect(loaded.has_value(), "game loop should persist configured run trace");
	Expect(loaded.has_value() && !loaded->empty() && (*loaded)[0] == "run frames=1 frameReports=1 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "game loop run trace should include run summary");
	Expect(loaded.has_value() && ContainsLineFragment(*loaded, "frame[0]"), "game loop run trace should include frame trace header");

	std::filesystem::remove_all(root);
}

void TestGameLoopSavesRunTraceOnStartupFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_failed_startup_trace_test";
	const std::filesystem::path tracePath = root / "failed-startup.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = root / "missing.iscl" },
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> loaded = dev::RuntimeFrameTraceFileStore {}.load(tracePath);

	Expect(result.setup.startupScriptRan, "failed startup trace test should attempt startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "failed startup trace test should report load failure");
	Expect(result.output.runTraceSaveAttempted, "game loop should attempt run trace save after startup failure");
	Expect(result.output.runTraceSaved, "game loop should save run trace after startup failure");
	Expect(loaded.has_value() && !loaded->empty() && (*loaded)[0] == "run frames=0 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "failed startup trace should preserve zero-frame summary");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsRunTraceSaveFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_run_trace_failure_test";
	const std::filesystem::path tracePath = root / "missing-parent" / "run.trace";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 0 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.output.runTraceSaveAttempted, "game loop should attempt configured run trace even when path is invalid");
	Expect(!result.output.runTraceSaved, "game loop should report failed run trace save");

	dev::GameLoop exitLoop {
		dev::GameLoopSettings {
		    .saveRoot = root / "other-saves",
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 0 },
		}
	};
	Expect(exitLoop.run() == 1, "game loop run should fail when requested trace cannot be saved");

	std::filesystem::remove_all(root);
}

void TestGameLoopSavesConfiguredDebugBundle()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_debug_bundle_test";
	const std::filesystem::path bundleRoot = root / "debug-bundle";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .output = { .debugBundlePath = bundleRoot },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "run.trace");

	Expect(result.output.debugBundleSaveAttempted, "game loop should attempt configured debug bundle save");
	Expect(result.output.debugBundleSaved, "game loop should report successful debug bundle save");
	Expect(manifest.has_value(), "game loop debug bundle should write manifest");
	Expect(trace.has_value(), "game loop debug bundle should write run trace");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "game loop debug bundle manifest should index saved trace");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "policy latest=Gameplay acceptCommands=true updatePlayers=true updateEnemies=true"), "game loop debug bundle manifest should include latest frame policy");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=1 frameReports=1 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "game loop debug bundle trace should include run summary");

	std::filesystem::remove_all(root);
}

void TestGameLoopSavesDebugBundleOnStartupFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_failed_startup_debug_bundle_test";
	const std::filesystem::path bundleRoot = root / "debug-bundle";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = root / "missing.iscl" },
		    .output = { .debugBundlePath = bundleRoot },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "run.trace");

	Expect(result.setup.startupScriptRan, "failed startup debug bundle test should attempt startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "failed startup debug bundle test should report load failure");
	Expect(result.output.debugBundleSaveAttempted, "game loop should attempt debug bundle save after startup failure");
	Expect(result.output.debugBundleSaved, "game loop should save debug bundle after startup failure");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "setup startupScriptRan=true inventoryScriptRan=false movementScriptRan=false"), "failed startup debug bundle manifest should record setup attempt");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "policy latest=none"), "failed startup debug bundle manifest should report no frame policy");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=0 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "failed startup debug bundle trace should preserve zero-frame summary");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsDebugBundleSaveFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_debug_bundle_failure_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root.parent_path());
	{
		std::ofstream output { root, std::ios::trunc };
		output << "not a directory\n";
	}

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root.parent_path() / "saves",
		    .output = { .debugBundlePath = root },
		    .frame = { .maxFrames = 0 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.output.debugBundleSaveAttempted, "game loop should attempt configured debug bundle even when path is invalid");
	Expect(!result.output.debugBundleSaved, "game loop should report failed debug bundle save");

	dev::GameLoop exitLoop {
		dev::GameLoopSettings {
		    .saveRoot = root.parent_path() / "other-saves",
		    .output = { .debugBundlePath = root },
		    .frame = { .maxFrames = 0 },
		}
	};
	Expect(exitLoop.run() == 1, "game loop run should fail when requested debug bundle cannot be saved");

	std::filesystem::remove(root);
}

} // namespace

int main()
{
	TestGameLoopSavesConfiguredRunTrace();
	TestGameLoopSavesRunTraceOnStartupFailure();
	TestGameLoopReportsRunTraceSaveFailure();
	TestGameLoopSavesConfiguredDebugBundle();
	TestGameLoopSavesDebugBundleOnStartupFailure();
	TestGameLoopReportsDebugBundleSaveFailure();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
