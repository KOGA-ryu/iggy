#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "app/RuntimeFrameLoopRunner.hpp"
#include "app/RuntimeFrameRunner.hpp"
#include "app/RuntimeInputSourceRouter.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunExecutor.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSetupRunner.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInputSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
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

void TestRuntimeFrameRunnerRoutesSourcesAndRecordsOneFrame()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_runner_test";
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
	    .code = 'P',
	    .pressed = true,
	});

	dev::GameLoopResult result;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
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
	dev::RuntimeFrameRunner {
		session,
		inputSourceRouter,
		sourceDrainer,
		recorder,
		dispatcher,
		dev::RuntimeFrameSettings { .maxFrames = 1 },
	}.runFrame();

	Expect(result.summary.framesRun == 1, "runtime frame runner should finish one frame");
	Expect(result.summary.rawInputEventsRouted == 1, "runtime frame runner should route raw input during the frame");
	Expect(result.summary.sessionCommandResults.size() == 1, "runtime frame runner should drain routed session commands");
	Expect(result.summary.sessionCommandResults.size() == 1 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "runtime frame runner should dispatch routed session commands");
	Expect(result.frameReports.size() == 1, "runtime frame runner should record one frame report");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].rawInputEventsRouted == 1, "runtime frame runner report should include raw input count");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].sessionCommandResults.size() == 1, "runtime frame runner report should include session command results");
	Expect(rawInput.empty(), "runtime frame runner should drain raw input sources once");
	Expect(routedSessionCommands.empty(), "runtime frame runner should drain routed session queue");
	Expect(session.mode() == dev::GameSessionMode::Paused, "runtime frame runner should apply routed pause command before simulation update");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameLoopRunnerRunsConfiguredFrames()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_loop_runner_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::GameLoopResult result;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSourceSettings sources;
	dev::RuntimeInputSettings input;
	dev::RuntimeFrameSettings frame {
	    .maxFrames = 3,
	    .fixedDeltaSeconds = 1.0F / 120.0F,
	};
	dev::RuntimeInputSourceRouter inputSourceRouter {
		session,
		routedSessionCommands,
		routedMovementCommands,
		sources,
		input,
	};
	dev::RuntimeFrameRunner frameRunner {
		session,
		inputSourceRouter,
		sourceDrainer,
		recorder,
		dispatcher,
		frame,
	};

	const int framesRun = dev::RuntimeFrameLoopRunner { frameRunner, frame }.run();

	Expect(framesRun == 3, "runtime frame loop runner should report configured frames run");
	Expect(result.summary.framesRun == 3, "runtime frame loop runner should run the configured frame count");
	Expect(result.frameReports.size() == 3, "runtime frame loop runner should record one report per frame");

	std::filesystem::remove_all(root);
}

void TestRuntimeRunExecutorRunsFramesOnlyWhenSetupAllows()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_run_executor_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::GameLoopResult result;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeInputSourceRouter inputSourceRouter {
		session,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceSettings {},
		dev::RuntimeInputSettings {},
	};
	dev::RuntimeFrameSettings frame {
	    .maxFrames = 2,
	};
	dev::RuntimeFrameRunner frameRunner {
		session,
		inputSourceRouter,
		sourceDrainer,
		recorder,
		dispatcher,
		frame,
	};
	dev::RuntimeFrameLoopRunner frameLoopRunner { frameRunner, frame };
	dev::RuntimeSetupRunner setupRunner { dispatcher, sourceDrainer };

	dev::RuntimeRunExecutor {
		session,
		recorder,
		setupRunner,
		frameLoopRunner,
	}.run(
	    result,
	    dev::RuntimeSetupSettings {},
	    dev::RuntimeOutputSettings {});

	Expect(result.summary.framesRun == 2, "runtime run executor should run frames when setup allows them");
	Expect(result.finalMode == dev::GameSessionMode::Empty, "runtime run executor should finalize the run after frames");
	Expect(!result.setup.startupScriptRan, "runtime run executor should preserve setup results");

	std::filesystem::remove_all(root);
}

void TestRuntimeRunExecutorFinalizesWithoutFramesAfterSetupFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_run_executor_failure_test";
	const std::filesystem::path missingStartup = root / "missing.iscl";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::GameLoopResult result;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeInputSourceRouter inputSourceRouter {
		session,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceSettings {},
		dev::RuntimeInputSettings {},
	};
	dev::RuntimeFrameSettings frame {
	    .maxFrames = 2,
	};
	dev::RuntimeFrameRunner frameRunner {
		session,
		inputSourceRouter,
		sourceDrainer,
		recorder,
		dispatcher,
		frame,
	};
	dev::RuntimeFrameLoopRunner frameLoopRunner { frameRunner, frame };
	dev::RuntimeSetupRunner setupRunner { dispatcher, sourceDrainer };

	dev::RuntimeRunExecutor {
		session,
		recorder,
		setupRunner,
		frameLoopRunner,
	}.run(
	    result,
	    dev::RuntimeSetupSettings { .startupScript = missingStartup },
	    dev::RuntimeOutputSettings {});

	Expect(result.setup.startupScriptRan, "runtime run executor should record attempted setup scripts");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "runtime run executor should preserve setup failure");
	Expect(result.summary.framesRun == 0, "runtime run executor should not run frames after setup failure");
	Expect(result.finalMode == dev::GameSessionMode::Empty, "runtime run executor should finalize even when frames are blocked");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestRuntimeFrameRunnerRoutesSourcesAndRecordsOneFrame();
	TestRuntimeFrameLoopRunnerRunsConfiguredFrames();
	TestRuntimeRunExecutorRunsFramesOnlyWhenSetupAllows();
	TestRuntimeRunExecutorFinalizesWithoutFramesAfterSetupFailure();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
