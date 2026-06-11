#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include "app/RuntimeInputTypes.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeSourceContext.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "app/RuntimeSourceDrainerSettingsBuilder.hpp"
#include "app/RuntimeSourceStream.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInputSource.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogFileStore.hpp"
#include "replay/MovementScriptSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandSource.hpp"
#include "simulation/SimulationFrameEvents.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestRuntimeSourceDrainerSettingsBuilderMapsLoopSourcesAndPlayer()
{
	dev::QueuedRawInputSource rawInput;
	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::QueuedMovementScriptSource movementScripts;
	dev::QueuedInventoryCommandSource inventoryCommands;
	dev::QueuedInventoryScriptSource inventoryScripts;

	dev::RuntimeSourceDrainerSettings settings = dev::RuntimeSourceDrainerSettingsBuilder {}.build(
	    dev::RuntimeSourceSettings {
	        .rawInputSources = { &rawInput },
	        .sessionCommandSources = { &sessionCommands },
	        .movementCommandSources = { &movementCommands },
	        .movementScriptSources = { &movementScripts },
	        .inventoryCommandSources = { &inventoryCommands },
	        .inventoryScriptSources = { &inventoryScripts },
	    },
	    dev::RuntimeInputSettings {
	        .playerId = 3,
	    });

	Expect(settings.sessionCommandSources.size() == 1 && settings.sessionCommandSources[0] == &sessionCommands, "runtime source drainer settings builder should copy session sources");
	Expect(settings.movementCommandSources.size() == 1 && settings.movementCommandSources[0] == &movementCommands, "runtime source drainer settings builder should copy movement sources");
	Expect(settings.movementScriptSources.size() == 1 && settings.movementScriptSources[0] == &movementScripts, "runtime source drainer settings builder should copy movement script sources");
	Expect(settings.inventoryCommandSources.size() == 1 && settings.inventoryCommandSources[0] == &inventoryCommands, "runtime source drainer settings builder should copy inventory command sources");
	Expect(settings.inventoryScriptSources.size() == 1 && settings.inventoryScriptSources[0] == &inventoryScripts, "runtime source drainer settings builder should copy inventory script sources");
	Expect(settings.inputPlayerId == 3, "runtime source drainer settings builder should map input player id to source-drainer player id");
}

void TestRuntimeSourceContextReportsActiveWorldAndPlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_source_context_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::RuntimeSourceContext emptyContext { session, 0 };
	Expect(!emptyContext.hasActiveWorld(), "runtime source context should report missing active world");
	Expect(!emptyContext.hasActivePlayer(), "runtime source context should report no active player without a world");
	Expect(emptyContext.activeWorld() == nullptr, "runtime source context should expose null active world pointer before a world exists");
	Expect(emptyContext.activePlayer() == nullptr, "runtime source context should expose null active player pointer before a world exists");

	session.startNewGame({ .playerStart = { 1, 2 }, .playerHitPoints = 20 });
	dev::RuntimeSourceContext playerContext { session, 0 };
	dev::RuntimeSourceContext missingPlayerContext { session, 3 };

	Expect(playerContext.hasActiveWorld(), "runtime source context should report active world");
	Expect(playerContext.hasActivePlayer(), "runtime source context should report valid input player");
	Expect(playerContext.activeWorld() != nullptr && playerContext.activeWorld()->players.size() == 1, "runtime source context should expose active world pointer");
	Expect(playerContext.activePlayer() != nullptr && playerContext.activePlayer()->position.tile == dev::Point { 1, 2 }, "runtime source context should expose selected player pointer");
	Expect(playerContext.world().players.size() == 1, "runtime source context should expose active world");
	Expect(playerContext.player().position.tile == dev::Point { 1, 2 }, "runtime source context should expose selected player");
	Expect(!missingPlayerContext.hasActivePlayer(), "runtime source context should reject out-of-range input player");
	Expect(missingPlayerContext.activeWorld() != nullptr, "runtime source context should still expose active world when selected player is missing");
	Expect(missingPlayerContext.activePlayer() == nullptr, "runtime source context should expose null active player pointer for out-of-range player");

	std::filesystem::remove_all(root);
}

void TestRuntimeSourceStreamDrainsSourcesAndSkipsNullSlots()
{
	dev::QueuedMovementCommandSource first;
	first.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::QueuedMovementCommandSource second;
	second.enqueue({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 2, 0 },
	});

	std::vector<dev::MovementCommand> commands = dev::RuntimeSourceStream<dev::MovementCommand, dev::MovementCommandSource> {}.drain(
	    first,
	    { nullptr, &second });

	Expect(commands.size() == 2, "runtime source stream should drain first and configured sources");
	Expect(commands.size() == 2 && commands[0].destination == dev::Point { 1, 0 }, "runtime source stream should preserve first source order");
	Expect(commands.size() == 2 && commands[1].destination == dev::Point { 2, 0 }, "runtime source stream should preserve configured source order");
	Expect(first.empty(), "runtime source stream should drain the first source once");
	Expect(second.empty(), "runtime source stream should drain configured sources once");
}

void TestRuntimeSourceDrainerDrainsSessionBeforeMovement()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_source_drainer_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;

	sessionCommands.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceDrainerSettings {
		    .sessionCommandSources = { &sessionCommands },
		    .movementCommandSources = { &movementCommands },
		},
	};
	dev::SessionCommandDispatcher dispatcher { session };

	std::vector<dev::SessionCommandResult> sessionResults = drainer.drainSessionCommands(dispatcher);
	int queuedMovement = drainer.drainMovementCommands();
	dev::SimulationFrameEvents frameEvents = session.update(1.0F / 60.0F);

	Expect(sessionResults.size() == 1 && sessionResults[0].type == dev::SessionCommandResultType::Applied, "runtime source drainer should dispatch session sources");
	Expect(session.hasActiveWorld(), "runtime source drainer session commands should create an active world");
	Expect(queuedMovement == 1, "runtime source drainer should queue movement after world exists");
	Expect(!frameEvents.movementEvents().empty(), "runtime source drainer test should produce movement frame events");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 1, 0 }, "runtime source drainer movement queue should feed session update");
	Expect(sessionCommands.empty(), "runtime source drainer should drain session source");
	Expect(movementCommands.empty(), "runtime source drainer should drain movement source");

	std::filesystem::remove_all(root);
}

void TestRuntimeSourceDrainerRunsMovementScripts()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_source_drainer_movement_script_test";
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
	Expect(store.save(scriptPath, log), "runtime movement script drainer test should create script file");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedMovementScriptSource movementScripts;
	movementScripts.enqueue(scriptPath);
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceDrainerSettings {
		    .movementScriptSources = { &movementScripts },
		},
	};

	std::vector<dev::MovementScriptRunResult> results = drainer.drainMovementScripts();
	dev::SimulationFrameEvents frameEvents = session.update(1.0F / 60.0F);

	Expect(results.size() == 1, "runtime source drainer should run movement script sources");
	Expect(results.size() == 1 && results[0].status == dev::MovementScriptRunStatus::Completed, "runtime source drainer movement script should complete");
	Expect(results.size() == 1 && results[0].replayReport.acceptedCount() == 1, "runtime source drainer movement script should report accepted command");
	Expect(frameEvents.movementEvents().size() == 1 && frameEvents.movementEvents()[0].type == dev::MovementEventType::StepCommitted, "runtime movement script should feed next simulation update");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 1, 0 }, "runtime movement script should move player through active world");
	Expect(movementScripts.empty(), "runtime source drainer should drain movement script source");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestRuntimeSourceDrainerSettingsBuilderMapsLoopSourcesAndPlayer();
	TestRuntimeSourceContextReportsActiveWorldAndPlayer();
	TestRuntimeSourceStreamDrainsSourcesAndSkipsNullSlots();
	TestRuntimeSourceDrainerDrainsSessionBeforeMovement();
	TestRuntimeSourceDrainerRunsMovementScripts();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
