#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "app/RuntimeMovementScriptBatchRunner.hpp"
#include "app/RuntimeMovementScriptIntake.hpp"
#include "app/RuntimeSessionCommandIntake.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "commands/MovementCommandSource.hpp"
#include "events/EventRecorder.hpp"
#include "player/Player.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogFileStore.hpp"
#include "replay/MovementScriptSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandSource.hpp"
#include "session/SessionEventRecorder.hpp"
#include "simulation/SimulationWorld.hpp"

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

void TestQueuedSessionCommandSourceDrainsCommandsOnce()
{
	dev::QueuedSessionCommandSource source;
	source.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 5, 5 } },
	});
	source.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});

	Expect(source.size() == 2, "queued session command source should track queued command count");
	std::vector<dev::SessionCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued session command source should drain queued commands");
	Expect(source.empty(), "queued session command source should be empty after drain");
	Expect(source.drain().empty(), "queued session command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].newGameSettings->playerStart == dev::Point { 5, 5 }, "queued session command source should preserve command payloads");
}

void TestQueuedMovementScriptSourceDrainsPathsOnce()
{
	dev::QueuedMovementScriptSource source;
	const std::filesystem::path first = "opening.imcl";
	const std::filesystem::path second = "combat.imcl";
	source.enqueue(first);
	source.enqueue(second);

	Expect(source.size() == 2, "queued movement script source should track queued path count");
	std::vector<std::filesystem::path> drained = source.drain();
	Expect(drained.size() == 2, "queued movement script source should drain queued paths");
	Expect(source.empty(), "queued movement script source should be empty after drain");
	Expect(source.drain().empty(), "queued movement script source should not drain paths twice");
	Expect(drained.size() == 2 && drained[0] == first, "queued movement script source should preserve first path");
	Expect(drained.size() == 2 && drained[1] == second, "queued movement script source should preserve second path");
	source.enqueue(first);
	source.clear();
	Expect(source.empty(), "queued movement script source should clear queued paths");
}

void TestRuntimeSessionCommandIntakeDispatchesCommandsInOrder()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_session_command_intake_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	std::vector<dev::SessionCommandResult> results = dev::RuntimeSessionCommandIntake {}.dispatch(
	    {
	        {
	            .type = dev::SessionCommandType::StartNewGame,
	            .newGameSettings = dev::NewGameSettings { .playerStart = { 3, 4 }, .playerHitPoints = 12 },
	        },
	        {
	            .type = dev::SessionCommandType::SetMode,
	            .mode = dev::GameSessionMode::Inventory,
	        },
	    },
	    dispatcher);

	Expect(results.size() == 2, "runtime session command intake should dispatch every drained command");
	Expect(results.size() == 2 && results[0].command.type == dev::SessionCommandType::StartNewGame, "runtime session command intake should preserve first command order");
	Expect(results.size() == 2 && results[1].command.type == dev::SessionCommandType::SetMode, "runtime session command intake should preserve second command order");
	Expect(results.size() == 2 && results[0].type == dev::SessionCommandResultType::Applied, "runtime session command intake should apply start command");
	Expect(results.size() == 2 && results[1].type == dev::SessionCommandResultType::Applied, "runtime session command intake should apply mode command");
	Expect(session.hasActiveWorld(), "runtime session command intake should mutate session through dispatcher");
	Expect(session.mode() == dev::GameSessionMode::Inventory, "runtime session command intake should leave session in dispatched mode");
	Expect(events.events().size() == 2 && events.events()[0].commandType == dev::SessionCommandType::StartNewGame, "runtime session command intake should emit first lifecycle event");
	Expect(events.events().size() == 2 && events.events()[1].commandType == dev::SessionCommandType::SetMode, "runtime session command intake should emit second lifecycle event");

	std::filesystem::remove_all(root);
}

void TestRuntimeMovementScriptIntakeRunsScriptsAgainstActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_movement_script_intake_test";
	const std::filesystem::path scriptPath = root / "runtime_movement.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog log;
	log.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	log.record({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime movement script intake test should save script");

	dev::EventRecorder events;
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.movementEvents = &events;

	const dev::MovementScriptRunResult result = dev::RuntimeMovementScriptIntake {}.run(scriptPath, &world);
	const dev::MovementScriptRunResult missingWorld = dev::RuntimeMovementScriptIntake {}.run(scriptPath, nullptr);

	Expect(result.status == dev::MovementScriptRunStatus::Completed, "runtime movement script intake should run scripts against active world");
	Expect(result.replayReport.results.size() == 2, "runtime movement script intake should replay every command");
	Expect(result.replayReport.acceptedCount() == 2, "runtime movement script intake should report accepted replay commands");
	Expect(world.players[0].moveState == dev::PlayerMoveState::Idle, "runtime movement script intake should let script dispatch affect world player state");
	Expect(events.events().size() >= 2, "runtime movement script intake should emit movement events through the world sink");
	Expect(missingWorld.status == dev::MovementScriptRunStatus::NoActiveWorld, "runtime movement script intake should report missing active world");
	Expect(missingWorld.replayReport.results.empty(), "runtime movement script intake should not replay without active world");

	std::filesystem::remove_all(root);
}

void TestRuntimeMovementScriptBatchRunnerPreservesPathOrder()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_movement_script_batch_runner_test";
	const std::filesystem::path firstPath = root / "first.imcl";
	const std::filesystem::path missingPath = root / "missing.imcl";
	const std::filesystem::path thirdPath = root / "third.imcl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::CommandLog firstLog;
	firstLog.record({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	dev::CommandLog thirdLog;
	thirdLog.record({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 0, 0 },
	});
	dev::CommandLogFileStore store;
	Expect(store.save(firstPath, firstLog), "runtime movement script batch runner test should save first script");
	Expect(store.save(thirdPath, thirdLog), "runtime movement script batch runner test should save third script");

	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	std::vector<dev::MovementScriptRunResult> results = dev::RuntimeMovementScriptBatchRunner {}.run(
	    { firstPath, missingPath, thirdPath },
	    world);

	Expect(results.size() == 3, "runtime movement script batch runner should produce one result per script path");
	Expect(results.size() == 3 && results[0].status == dev::MovementScriptRunStatus::Completed, "runtime movement script batch runner should keep first script result order");
	Expect(results.size() == 3 && results[1].status == dev::MovementScriptRunStatus::LoadFailed, "runtime movement script batch runner should keep missing script result order");
	Expect(results.size() == 3 && results[2].status == dev::MovementScriptRunStatus::Completed, "runtime movement script batch runner should continue after load failure");
	Expect(results.size() == 3 && results[0].replayReport.acceptedCount() == 1, "runtime movement script batch runner should replay first script");
	Expect(results.size() == 3 && results[2].replayReport.results.size() == 1, "runtime movement script batch runner should replay later scripts");

	std::filesystem::remove_all(root);
}

void TestRuntimeSourceSettingsDefaultsToNoSources()
{
	dev::RuntimeSourceSettings sources;

	Expect(sources.rawInputSources.empty(), "runtime source settings should default to no raw input sources");
	Expect(sources.sessionCommandSources.empty(), "runtime source settings should default to no session command sources");
	Expect(sources.movementCommandSources.empty(), "runtime source settings should default to no movement command sources");
	Expect(sources.inventoryCommandSources.empty(), "runtime source settings should default to no inventory command sources");
	Expect(sources.inventoryScriptSources.empty(), "runtime source settings should default to no inventory script sources");
}

} // namespace

int main()
{
	TestQueuedSessionCommandSourceDrainsCommandsOnce();
	TestQueuedMovementScriptSourceDrainsPathsOnce();
	TestRuntimeSessionCommandIntakeDispatchesCommandsInOrder();
	TestRuntimeMovementScriptIntakeRunsScriptsAgainstActiveWorld();
	TestRuntimeMovementScriptBatchRunnerPreservesPathOrder();
	TestRuntimeSourceSettingsDefaultsToNoSources();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
