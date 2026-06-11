#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/RuntimeFrameSourcePhaseRunner.hpp"
#include "app/RuntimeInputSourceRouter.hpp"
#include "app/RuntimeInventoryFrameSourceStep.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeMovementFrameSourceStep.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSessionFrameSourceStep.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInputSource.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogFileStore.hpp"
#include "replay/MovementScriptSource.hpp"
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

void TestRuntimeSessionFrameSourceStepRoutesRawInputBeforeSessionSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_session_frame_source_step_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'I',
	    .pressed = true,
	});
	dev::QueuedSessionCommandSource sessionCommands;
	sessionCommands.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	dev::RuntimeSourceSettings sources {
	    .rawInputSources = { &rawInput },
	};
	dev::RuntimeInputSettings input {
	    .bindings = dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'S' },
	};
	dev::RuntimeInputSourceRouter inputSourceRouter {
		session,
		routedSessionCommands,
		routedMovementCommands,
		sources,
		input,
	};
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceDrainerSettings {
		    .sessionCommandSources = { &sessionCommands },
		},
	};
	dev::GameLoopResult result;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	recorder.beginFrame();

	dev::RuntimeSessionFrameSourceStep {}.run(
	    inputSourceRouter,
	    sourceDrainer,
	    recorder,
	    dispatcher);
	recorder.finishFrame();

	Expect(result.summary.rawInputEventsRouted == 1, "runtime session frame source step should record routed raw input");
	Expect(result.summary.sessionCommandResults.size() == 2, "runtime session frame source step should dispatch routed and configured session commands");
	Expect(result.summary.sessionCommandResults[0].command.mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "runtime session frame source step should dispatch raw-input session commands first");
	Expect(result.summary.sessionCommandResults[1].command.mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Paused }, "runtime session frame source step should dispatch configured session sources after routed commands");
	Expect(session.mode() == dev::GameSessionMode::Paused, "runtime session frame source step should leave session after ordered lifecycle commands");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].rawInputEventsRouted == 1, "runtime session frame source step should record frame raw input count");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].sessionCommandResults.size() == 2, "runtime session frame source step should record frame session command results");
	Expect(rawInput.empty(), "runtime session frame source step should drain raw input sources");
	Expect(routedSessionCommands.empty(), "runtime session frame source step should drain routed session commands");
	Expect(sessionCommands.empty(), "runtime session frame source step should drain configured session command sources");

	std::filesystem::remove_all(root);
}

void TestRuntimeMovementFrameSourceStepRunsScriptsBeforeQueueingCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_movement_frame_source_step_test";
	const std::filesystem::path scriptPath = root / "movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime movement frame source step test should create movement script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedMovementScriptSource movementScripts;
	movementScripts.enqueue(scriptPath);
	dev::QueuedMovementCommandSource movementCommands;
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 2, 0 },
	});
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceDrainerSettings {
		    .movementScriptSources = { &movementScripts },
		    .movementCommandSources = { &movementCommands },
		},
	};
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	recorder.beginFrame();

	dev::RuntimeMovementFrameSourceStep {}.run(sourceDrainer, recorder);
	recorder.finishFrame();

	dev::MovementCommand queued {};

	Expect(result.summary.runtimeMovementScriptResults.size() == 1, "runtime movement frame source step should record movement script results");
	Expect(result.summary.runtimeMovementScriptResults[0].status == dev::MovementScriptRunStatus::Completed, "runtime movement frame source step should run movement scripts");
	Expect(result.summary.runtimeMovementScriptResults[0].replayReport.acceptedCount() == 1, "runtime movement frame source step should preserve script replay results");
	Expect(result.summary.movementCommandsQueued == 1, "runtime movement frame source step should record queued direct movement commands");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].movementScriptResults.size() == 1, "runtime movement frame source step should record frame movement script results");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].movementCommandsQueued == 1, "runtime movement frame source step should record frame queued movement count");
	Expect(movementScripts.empty(), "runtime movement frame source step should drain movement script sources");
	Expect(movementCommands.empty(), "runtime movement frame source step should drain movement command sources");
	Expect(session.world().commandQueue.tryPop(queued) && queued.destination == dev::Point { 2, 0 }, "runtime movement frame source step should queue direct movement commands into the active world");

	std::filesystem::remove_all(root);
}

void TestRuntimeInventoryFrameSourceStepDrainsScriptsBeforeCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_inventory_frame_source_step_test";
	const std::filesystem::path scriptPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 71,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime inventory frame source step test should create inventory script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	session.world().players[0].inventory.items.push_back({
	    .id = 71,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	session.world().players[0].inventory.items.push_back({
	    .id = 72,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);
	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 72,
	});
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceDrainerSettings {
		    .inventoryScriptSources = { &inventoryScripts },
		    .inventoryCommandSources = { &inventoryCommands },
		},
	};
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	recorder.beginFrame();

	dev::RuntimeInventoryFrameSourceStep {}.run(sourceDrainer, recorder);
	recorder.finishFrame();

	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "runtime inventory frame source step should record inventory script results");
	Expect(result.summary.inventoryCommandResults.size() == 2, "runtime inventory frame source step should record script and direct inventory command results");
	Expect(result.summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 71 }, "runtime inventory frame source step should record script command results before direct commands");
	Expect(result.summary.inventoryCommandResults[1].command.itemId == std::optional<dev::TargetId> { 72 }, "runtime inventory frame source step should record direct commands after scripts");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].inventoryScriptResults.size() == 1, "runtime inventory frame source step should record frame script results");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].inventoryCommandResults.size() == 2, "runtime inventory frame source step should record frame inventory command results");
	Expect(inventoryScripts.empty(), "runtime inventory frame source step should drain inventory script sources");
	Expect(inventoryCommands.empty(), "runtime inventory frame source step should drain inventory command sources");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameSourcePhaseRunnerRoutesAndDrainsSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_source_phase_runner_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	session.world().players[0].inventory.items.push_back({
	    .id = 61,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'I',
	    .pressed = true,
	});
	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 61,
	});
	dev::QueuedMovementCommandSource movementCommands;
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoopResult result;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	recorder.beginFrame();
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceDrainerSettings {
		    .inventoryCommandSources = { &inventoryCommands },
		    .movementCommandSources = { &movementCommands },
		},
	};
	dev::RuntimeSourceSettings sources {
	    .rawInputSources = { &rawInput },
	};
	dev::RuntimeInputSettings input {
	    .bindings = dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'S' },
	};
	dev::RuntimeInputSourceRouter inputSourceRouter {
		session,
		routedSessionCommands,
		routedMovementCommands,
		sources,
		input,
	};

	dev::RuntimeFrameSourcePhaseRunner {}.run(
	    inputSourceRouter,
	    sourceDrainer,
	    recorder,
	    dispatcher);

	Expect(result.summary.rawInputEventsRouted == 1, "runtime frame source phase runner should record routed raw input");
	Expect(result.summary.sessionCommandResults.size() == 1 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "runtime frame source phase runner should dispatch routed session command");
	Expect(session.mode() == dev::GameSessionMode::Inventory, "runtime frame source phase runner should apply session commands before later frame work");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime frame source phase runner should dispatch inventory command sources");
	Expect(result.summary.movementCommandsQueued == 1, "runtime frame source phase runner should queue movement command sources");
	Expect(rawInput.empty(), "runtime frame source phase runner should drain raw input source");
	Expect(routedSessionCommands.empty(), "runtime frame source phase runner should drain routed session queue");
	Expect(inventoryCommands.empty(), "runtime frame source phase runner should drain inventory command source");
	Expect(movementCommands.empty(), "runtime frame source phase runner should drain movement command source");
	dev::MovementCommand queued {};
	Expect(session.world().commandQueue.tryPop(queued) && queued.destination == dev::Point { 1, 0 }, "runtime frame source phase runner should queue movement commands into active world");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestRuntimeSessionFrameSourceStepRoutesRawInputBeforeSessionSources();
	TestRuntimeMovementFrameSourceStepRunsScriptsBeforeQueueingCommands();
	TestRuntimeInventoryFrameSourceStepDrainsScriptsBeforeCommands();
	TestRuntimeFrameSourcePhaseRunnerRoutesAndDrainsSources();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
