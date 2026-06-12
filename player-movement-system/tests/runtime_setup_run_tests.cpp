#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "app/RuntimeInventoryCommandReportRecorder.hpp"
#include "app/RuntimeInventoryScriptReportRecorder.hpp"
#include "app/RuntimeMovementScriptReportRecorder.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSessionCommandReportRecorder.hpp"
#include "app/RuntimeSetupFrameGate.hpp"
#include "app/RuntimeSetupInventoryCommandReportRecorder.hpp"
#include "app/RuntimeSetupRunResultApplier.hpp"
#include "app/RuntimeSetupRunner.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "commands/MovementCommandSource.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogFileStore.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandSource.hpp"
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

void TestRuntimeSetupSettingsDefaultsToNoScripts()
{
	dev::RuntimeSetupSettings setup;

	Expect(!setup.startupScript.has_value(), "runtime setup settings should default to no startup script");
	Expect(!setup.inventoryScript.has_value(), "runtime setup settings should default to no configured inventory script");
	Expect(!setup.movementScript.has_value(), "runtime setup settings should default to no configured movement script");
}

void TestRuntimeSetupResultDefaultsToNoSetupScripts()
{
	dev::RuntimeSetupResult setup;

	Expect(!setup.startupScriptRan, "runtime setup result should default to no startup script");
	Expect(setup.startupScriptResult.commandResults.empty(), "runtime setup result should default to no startup command results");
	Expect(!setup.inventoryScriptRan, "runtime setup result should default to no inventory script");
	Expect(setup.inventoryScriptResult.commandResults.empty(), "runtime setup result should default to no inventory command results");
	Expect(!setup.movementScriptRan, "runtime setup result should default to no movement script");
	Expect(setup.movementScriptResult.replayReport.results.empty(), "runtime setup result should default to no movement replay results");
}

void TestRuntimeSetupRunnerAllowsFramesWhenNoScriptsConfigured()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_empty_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({});

	Expect(result.framesAllowed, "runtime setup runner should allow frames when no setup scripts are configured");
	Expect(!result.setup.startupScriptRan, "runtime setup runner should not invent startup script attempts");
	Expect(!result.setup.inventoryScriptRan, "runtime setup runner should not invent inventory script attempts");
	Expect(!result.setup.movementScriptRan, "runtime setup runner should not invent movement script attempts");
	Expect(result.inventoryCommandResults.empty(), "runtime setup runner should report no setup inventory command results without scripts");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerStopsFramesAfterStartupLoadFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_startup_failure_test";
	const std::filesystem::path missingStartup = root / "missing.iscl";
	const std::filesystem::path missingInventory = root / "missing.iicl";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .startupScript = missingStartup,
	    .inventoryScript = missingInventory,
	});

	Expect(!result.framesAllowed, "runtime setup runner should stop frames after startup load failure");
	Expect(result.setup.startupScriptRan, "runtime setup runner should attempt configured startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "runtime setup runner should report startup load failure");
	Expect(!result.setup.inventoryScriptRan, "runtime setup runner should not run inventory setup after startup load failure");
	Expect(!result.setup.movementScriptRan, "runtime setup runner should not run movement setup after startup load failure");
	Expect(result.inventoryCommandResults.empty(), "failed startup setup should not produce inventory command results");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerPreservesInventoryCommandRejections()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_inventory_rejection_test";
	const std::filesystem::path inventoryPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog inventoryLog;
	inventoryLog.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 72,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(inventoryPath, inventoryLog), "runtime setup runner inventory rejection test should create inventory script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .inventoryScript = inventoryPath,
	});

	Expect(result.framesAllowed, "runtime setup runner should allow frames after loadable inventory script command rejection");
	Expect(result.setup.inventoryScriptRan, "runtime setup runner should attempt configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::Completed, "runtime setup runner should complete loadable inventory scripts");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1, "runtime setup runner should preserve inventory script command results");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1 && result.setup.inventoryScriptResult.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup runner should preserve rejected inventory command result");
	Expect(result.inventoryCommandResults.size() == 1 && result.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup runner should expose configured inventory command results for run summaries");
	Expect(inventoryEvents.events().size() == 1 && inventoryEvents.events()[0].type == dev::InventoryEventType::Rejected, "runtime setup runner should preserve inventory setup events");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerStopsFramesAfterInventorySetupFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_inventory_failure_test";
	const std::filesystem::path missingInventory = root / "missing.iicl";
	const std::filesystem::path movementPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog movementLog;
	movementLog.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore movementStore;
	Expect(movementStore.save(movementPath, movementLog), "runtime setup runner inventory failure test should create movement script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .inventoryScript = missingInventory,
	    .movementScript = movementPath,
	});

	Expect(!result.framesAllowed, "runtime setup runner should stop frames after inventory setup failure");
	Expect(result.setup.inventoryScriptRan, "runtime setup runner should attempt configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::LoadFailed, "runtime setup runner should report inventory setup load failure");
	Expect(!result.setup.movementScriptRan, "runtime setup runner should not run movement setup after inventory setup failure");
	Expect(result.inventoryCommandResults.empty(), "failed inventory setup should not expose setup inventory command results");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerPreservesMovementCommandRejections()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_movement_rejection_test";
	const std::filesystem::path movementPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog movementLog;
	movementLog.record({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});
	dev::CommandLogFileStore movementStore;
	Expect(movementStore.save(movementPath, movementLog), "runtime setup runner movement rejection test should create movement script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .movementScript = movementPath,
	});

	Expect(result.framesAllowed, "runtime setup runner should allow frames after loadable movement script command rejection");
	Expect(result.setup.movementScriptRan, "runtime setup runner should attempt configured movement script");
	Expect(result.setup.movementScriptResult.status == dev::MovementScriptRunStatus::Completed, "runtime setup runner should complete loadable movement scripts");
	Expect(result.setup.movementScriptResult.replayReport.results.size() == 1, "runtime setup runner should preserve movement replay results");
	Expect(result.setup.movementScriptResult.replayReport.rejectedCount() == 1, "runtime setup runner should preserve rejected movement commands");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerStopsFramesWithoutActiveWorldForMovementScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_movement_no_world_test";
	const std::filesystem::path movementPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog movementLog;
	movementLog.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore movementStore;
	Expect(movementStore.save(movementPath, movementLog), "runtime setup runner no-world movement test should create movement script");

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .movementScript = movementPath,
	});

	Expect(!result.framesAllowed, "runtime setup runner should stop frames when movement setup has no active world");
	Expect(result.setup.movementScriptRan, "runtime setup runner should attempt configured movement script without world");
	Expect(result.setup.movementScriptResult.status == dev::MovementScriptRunStatus::NoActiveWorld, "runtime setup runner should report movement setup without active world");

	std::filesystem::remove_all(root);
}

void TestRuntimeInventoryScriptReportRecorderKeepsScriptsAndFlattensCommands()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 10 },
	});
	summary.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 11 },
	});

	dev::RuntimeInventoryScriptReportRecorder {}.record(
	    {
	        {
	            .status = dev::InventoryScriptRunStatus::Completed,
	            .commandResults = {
	                {
	                    .type = dev::InventoryCommandResultType::Applied,
	                    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 12 },
	                    .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 12 },
	                },
	            },
	        },
	    },
	    frame,
	    summary);

	Expect(frame.inventoryScriptResults.size() == 1 && frame.inventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script report recorder should copy script results onto frame report");
	Expect(summary.runtimeInventoryScriptResults.size() == 1 && summary.runtimeInventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script report recorder should aggregate script results onto summary");
	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 10 }, "runtime inventory script report recorder should preserve existing frame command results");
	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[1].command.itemId == std::optional<dev::TargetId> { 12 }, "runtime inventory script report recorder should append script command results to frame");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 11 }, "runtime inventory script report recorder should preserve existing summary command results");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[1].command.itemId == std::optional<dev::TargetId> { 12 }, "runtime inventory script report recorder should append script command results to summary");
}

void TestRuntimeInventoryCommandReportRecorderAppendsDirectCommandResults()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 20 },
	});
	summary.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 21 },
	});

	dev::RuntimeInventoryCommandReportRecorder {}.record(
	    {
	        {
	            .type = dev::InventoryCommandResultType::Rejected,
	            .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
	            .equipmentResult = { .type = dev::EquipmentResultType::MissingItem },
	        },
	    },
	    frame,
	    summary);

	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 20 }, "runtime inventory command report recorder should preserve existing frame command results");
	Expect(frame.inventoryCommandResults.size() == 2 && frame.inventoryCommandResults[1].command.type == dev::InventoryCommandType::UnequipSlot, "runtime inventory command report recorder should append direct command result to frame");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 21 }, "runtime inventory command report recorder should preserve existing summary command results");
	Expect(summary.inventoryCommandResults.size() == 2 && summary.inventoryCommandResults[1].command.type == dev::InventoryCommandType::UnequipSlot, "runtime inventory command report recorder should append direct command result to summary");
}

void TestRuntimeSessionCommandReportRecorderReplacesFrameAndAggregatesSummary()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.sessionCommandResults.push_back({
	    .type = dev::SessionCommandResultType::Rejected,
	    .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Paused },
	});
	summary.sessionCommandResults.push_back({
	    .type = dev::SessionCommandResultType::Applied,
	    .command = { .type = dev::SessionCommandType::StartNewGame },
	});

	dev::RuntimeSessionCommandReportRecorder {}.record(
	    {
	        {
	            .type = dev::SessionCommandResultType::Applied,
	            .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Inventory },
	        },
	    },
	    frame,
	    summary);

	Expect(frame.sessionCommandResults.size() == 1 && frame.sessionCommandResults[0].command.mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "runtime session command report recorder should replace frame session results with current frame results");
	Expect(summary.sessionCommandResults.size() == 2 && summary.sessionCommandResults[0].command.type == dev::SessionCommandType::StartNewGame, "runtime session command report recorder should preserve existing summary session results");
	Expect(summary.sessionCommandResults.size() == 2 && summary.sessionCommandResults[1].command.mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "runtime session command report recorder should append current frame results to summary");
}

void TestRuntimeMovementScriptReportRecorderReplacesFrameAndAggregatesSummary()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.movementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::LoadFailed,
	});
	summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::NoActiveWorld,
	});

	dev::RuntimeMovementScriptReportRecorder {}.record(
	    {
	        {
	            .status = dev::MovementScriptRunStatus::Completed,
	        },
	    },
	    frame,
	    summary);

	Expect(frame.movementScriptResults.size() == 1 && frame.movementScriptResults[0].status == dev::MovementScriptRunStatus::Completed, "runtime movement script report recorder should replace frame movement scripts with current frame results");
	Expect(summary.runtimeMovementScriptResults.size() == 2 && summary.runtimeMovementScriptResults[0].status == dev::MovementScriptRunStatus::NoActiveWorld, "runtime movement script report recorder should preserve existing summary movement script results");
	Expect(summary.runtimeMovementScriptResults.size() == 2 && summary.runtimeMovementScriptResults[1].status == dev::MovementScriptRunStatus::Completed, "runtime movement script report recorder should append current frame movement script results");
	Expect(summary.movementCommandsQueued == 0, "runtime movement script report recorder should not inflate queued movement command count");
}

void TestRuntimeSetupInventoryCommandReportRecorderAppendsOnlySummaryResults()
{
	dev::RuntimeRunSummary summary;
	summary.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 1 },
	    .equipmentResult = { .type = dev::EquipmentResultType::MissingItem, .itemId = 1 },
	});

	dev::RuntimeSetupInventoryCommandReportRecorder {}.record(
	    {
	        {
	            .type = dev::InventoryCommandResultType::Applied,
	            .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 9 },
	            .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 9 },
	        },
	    },
	    summary);

	Expect(summary.inventoryCommandResults.size() == 2, "runtime setup inventory command report recorder should append setup results to existing summary results");
	Expect(summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup inventory command report recorder should preserve existing summary result order");
	Expect(summary.inventoryCommandResults[1].command.itemId == std::optional<dev::TargetId> { 9 }, "runtime setup inventory command report recorder should preserve setup command payload");
	Expect(summary.framesRun == 0, "runtime setup inventory command report recorder should not count frames");
}

void TestRuntimeSetupRunResultApplierCopiesSetupAndRecordsSummary()
{
	dev::RuntimeSetupRunResult setupResult;
	setupResult.setup.startupScriptRan = true;
	setupResult.setup.inventoryScriptRan = true;
	setupResult.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 12 },
	    .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 12 },
	});
	setupResult.framesAllowed = false;

	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	const bool framesAllowed = dev::RuntimeSetupRunResultApplier {}.apply(
	    setupResult,
	    result,
	    recorder);

	Expect(!framesAllowed, "runtime setup run result applier should return setup frame gate");
	Expect(result.setup.startupScriptRan, "runtime setup run result applier should copy startup setup result");
	Expect(result.setup.inventoryScriptRan, "runtime setup run result applier should copy inventory setup result");
	Expect(result.summary.inventoryCommandResults.size() == 1, "runtime setup run result applier should record setup inventory command results");
	Expect(result.summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 12 }, "runtime setup run result applier should preserve setup command payload");
	Expect(result.summary.framesRun == 0, "runtime setup run result applier should not finish frames");
}

void TestRuntimeFrameSettingsDefaultsToNoFramesAtSixtyHz()
{
	dev::RuntimeFrameSettings frame;

	Expect(frame.maxFrames == 0, "runtime frame settings should default to zero bounded frames");
	Expect(frame.fixedDeltaSeconds == 1.0F / 60.0F, "runtime frame settings should default to sixty hertz timestep");
}

void TestRuntimeRunSummaryDefaultsToEmptyRun()
{
	dev::RuntimeRunSummary summary;

	Expect(summary.runtimeInventoryScriptResults.empty(), "runtime run summary should default to no runtime inventory script results");
	Expect(summary.rawInputEventsRouted == 0, "runtime run summary should default to no routed raw input");
	Expect(summary.movementInputBlockReasons.empty(), "runtime run summary should default to no movement input block reasons");
	Expect(summary.sessionCommandResults.empty(), "runtime run summary should default to no session command results");
	Expect(summary.inventoryCommandResults.empty(), "runtime run summary should default to no inventory command results");
	Expect(summary.movementCommandsQueued == 0, "runtime run summary should default to no queued movement commands");
	Expect(summary.framesRun == 0, "runtime run summary should default to zero frames");
	Expect(summary.lastFrameEvents.movementEvents().empty(), "runtime run summary should default to no final movement events");
}

void TestRuntimeRunRecorderAggregatesSetupInventoryResultsWithoutFrame()
{
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	recorder.recordSetupInventoryCommandResults({
	    {
	        .type = dev::InventoryCommandResultType::Applied,
	        .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 73 },
	        .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 73 },
	    },
	});

	Expect(result.summary.inventoryCommandResults.size() == 1, "runtime run recorder should aggregate setup inventory command results");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 73 }, "runtime run recorder should preserve setup inventory result payload");
	Expect(result.summary.framesRun == 0, "runtime run recorder setup aggregation should not count a frame");
	Expect(result.frameReports.empty(), "runtime run recorder setup aggregation should not create a frame report");
}

void TestRuntimeRunRecorderAggregatesFrameReportsAndSummary()
{
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	sessionEvents.emit({
	    .type = dev::SessionEventType::GameStarted,
	    .commandType = dev::SessionCommandType::StartNewGame,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Rejected,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Rejected,
	    .equipmentResult = dev::EquipmentResultType::MissingItem,
	    .itemId = 70,
	});

	recorder.beginFrame();
	sessionEvents.emit({
	    .type = dev::SessionEventType::ModeChanged,
	    .commandType = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Equipped,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Applied,
	    .equipmentResult = dev::EquipmentResultType::Equipped,
	    .itemId = 71,
	});

	recorder.recordRawInputDrainResult({
	    .handled = 2,
	    .movementBlockReasons = { dev::PlayerActionBlockReason::Focus },
	});
	recorder.recordSessionCommandResults({
	    {
	        .type = dev::SessionCommandResultType::Applied,
	        .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Inventory },
	    },
	});
	recorder.recordInventoryScriptResults({
	    {
	        .status = dev::InventoryScriptRunStatus::Completed,
	        .commandResults = {
	            {
	                .type = dev::InventoryCommandResultType::Applied,
	                .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 71 },
	                .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 71 },
	            },
	        },
	    },
	});
	recorder.recordInventoryCommandResults({
	    {
	        .type = dev::InventoryCommandResultType::Applied,
	        .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
	        .equipmentResult = { .type = dev::EquipmentResultType::Unequipped, .itemId = 71 },
	    },
	});
	recorder.recordMovementCommandsQueued(1);
	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit({
	    .type = dev::MovementEventType::CommandAccepted,
	    .playerId = 0,
	    .tile = { 1, 0 },
	    .commandType = dev::MovementCommandType::WalkTo,
	});
	recorder.recordFrameEvents(frameEvents);
	recorder.finishFrame();

	Expect(result.summary.framesRun == 1, "runtime run recorder should count finished frames");
	Expect(result.summary.rawInputEventsRouted == 2, "runtime run recorder should aggregate routed raw input");
	Expect(result.summary.movementInputBlockReasons.size() == 1 && result.summary.movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "runtime run recorder should aggregate movement input block reasons");
	Expect(result.summary.sessionCommandResults.size() == 1, "runtime run recorder should aggregate session results");
	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "runtime run recorder should aggregate inventory script results");
	Expect(result.summary.inventoryCommandResults.size() == 2, "runtime run recorder should aggregate script and direct inventory results");
	Expect(result.summary.movementCommandsQueued == 1, "runtime run recorder should aggregate movement queue counts");
	Expect(result.summary.lastFrameEvents.movementEvents().size() == 1, "runtime run recorder should store final frame events");
	Expect(result.frameReports.size() == 1, "runtime run recorder should create one frame report");
	if (result.frameReports.empty())
		return;

	const dev::RuntimeFrameReport &report = result.frameReports[0];
	Expect(report.rawInputEventsRouted == 2, "runtime run recorder frame report should keep raw input count");
	Expect(report.movementInputBlockReasons.size() == 1 && report.movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "runtime run recorder frame report should keep movement input block reasons");
	Expect(report.sessionCommandResults.size() == 1, "runtime run recorder frame report should keep session results");
	Expect(report.inventoryScriptResults.size() == 1, "runtime run recorder frame report should keep inventory scripts");
	Expect(report.inventoryCommandResults.size() == 2, "runtime run recorder frame report should keep script and direct inventory results");
	Expect(report.movementCommandsQueued == 1, "runtime run recorder frame report should keep movement queue count");
	Expect(report.sessionEvents.size() == 1 && report.sessionEvents[0].type == dev::SessionEventType::ModeChanged, "runtime run recorder should capture session event deltas");
	Expect(report.inventoryEvents.size() == 1 && report.inventoryEvents[0].itemId == 71, "runtime run recorder should capture inventory event deltas");
	Expect(report.frameEvents.movementEvents().size() == 1, "runtime run recorder frame report should keep simulation frame events");
}

void TestRuntimeSetupFrameGateAllowsOnlyNonFatalSetupResults()
{
	dev::RuntimeSetupFrameGate gate;

	dev::RuntimeSetupResult clean;

	dev::RuntimeSetupResult commandRejection;
	commandRejection.inventoryScriptRan = true;
	commandRejection.inventoryScriptResult.status = dev::InventoryScriptRunStatus::Completed;
	commandRejection.inventoryScriptResult.commandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 4 },
	    .equipmentResult = { .type = dev::EquipmentResultType::MissingItem, .itemId = 4 },
	});

	dev::RuntimeSetupResult startupFailure;
	startupFailure.startupScriptRan = true;
	startupFailure.startupScriptResult.status = dev::SessionScriptRunStatus::LoadFailed;

	dev::RuntimeSetupResult movementFailure;
	movementFailure.movementScriptRan = true;
	movementFailure.movementScriptResult.status = dev::MovementScriptRunStatus::NoActiveWorld;

	Expect(gate.allowsFrames(clean), "runtime setup frame gate should allow frames after clean setup");
	Expect(gate.allowsFrames(commandRejection), "runtime setup frame gate should allow frames after command-level setup rejections");
	Expect(!gate.allowsFrames(startupFailure), "runtime setup frame gate should block frames after startup load failure");
	Expect(!gate.allowsFrames(movementFailure), "runtime setup frame gate should block frames after movement setup failure");
}

} // namespace

int main()
{
	TestRuntimeSetupSettingsDefaultsToNoScripts();
	TestRuntimeSetupResultDefaultsToNoSetupScripts();
	TestRuntimeSetupRunnerAllowsFramesWhenNoScriptsConfigured();
	TestRuntimeSetupRunnerStopsFramesAfterStartupLoadFailure();
	TestRuntimeSetupRunnerPreservesInventoryCommandRejections();
	TestRuntimeSetupRunnerStopsFramesAfterInventorySetupFailure();
	TestRuntimeSetupRunnerPreservesMovementCommandRejections();
	TestRuntimeSetupRunnerStopsFramesWithoutActiveWorldForMovementScript();
	TestRuntimeInventoryScriptReportRecorderKeepsScriptsAndFlattensCommands();
	TestRuntimeInventoryCommandReportRecorderAppendsDirectCommandResults();
	TestRuntimeSessionCommandReportRecorderReplacesFrameAndAggregatesSummary();
	TestRuntimeMovementScriptReportRecorderReplacesFrameAndAggregatesSummary();
	TestRuntimeSetupInventoryCommandReportRecorderAppendsOnlySummaryResults();
	TestRuntimeSetupRunResultApplierCopiesSetupAndRecordsSummary();
	TestRuntimeFrameSettingsDefaultsToNoFramesAtSixtyHz();
	TestRuntimeRunSummaryDefaultsToEmptyRun();
	TestRuntimeRunRecorderAggregatesSetupInventoryResultsWithoutFrame();
	TestRuntimeRunRecorderAggregatesFrameReportsAndSummary();
	TestRuntimeSetupFrameGateAllowsOnlyNonFatalSetupResults();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
