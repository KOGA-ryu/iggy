#include "RuntimeSourceDrainer.hpp"

#include "app/RuntimeSourceStream.hpp"
#include "commands/CommandDispatcher.hpp"
#include "player/PlayerController.hpp"

#include <utility>

namespace dev {

RuntimeSourceDrainer::RuntimeSourceDrainer(
    GameSession &session,
    InventoryEventRecorder &inventoryEvents,
    QueuedSessionCommandSource &routedSessionCommands,
    QueuedMovementCommandSource &routedMovementCommands,
    RuntimeSourceDrainerSettings settings)
    : session_(session)
    , inventoryEvents_(inventoryEvents)
    , routedSessionCommands_(routedSessionCommands)
    , routedMovementCommands_(routedMovementCommands)
    , settings_(std::move(settings))
{
}

InventoryScriptRunResult RuntimeSourceDrainer::runInventoryScript(const std::filesystem::path &path)
{
	if (!session_.hasActiveWorld() || settings_.inputPlayerId >= session_.world().players.size())
		return { .status = InventoryScriptRunStatus::NoActivePlayer };

	InventoryCommandDispatcher dispatcher { session_.world().players[settings_.inputPlayerId], &inventoryEvents_ };
	InventoryScriptRunner runner { dispatcher };
	return runner.run(path);
}

MovementScriptRunResult RuntimeSourceDrainer::runMovementScript(const std::filesystem::path &path)
{
	if (!session_.hasActiveWorld())
		return { .status = MovementScriptRunStatus::NoActiveWorld };

	PlayerController playerController {
		session_.world().players,
		session_.world().map,
		session_.world().collision,
		session_.world().pathFinder,
		session_.world().movementEvents,
	};
	CommandDispatcher dispatcher { playerController, session_.world().movementEvents };
	MovementScriptRunner runner { dispatcher };
	return runner.run(path);
}

std::vector<SessionCommandResult> RuntimeSourceDrainer::drainSessionCommands(const SessionCommandDispatcher &dispatcher)
{
	std::vector<SessionCommandResult> results;
	std::vector<SessionCommand> commands = RuntimeSourceStream<SessionCommand, SessionCommandSource> {}.drain(
	    routedSessionCommands_,
	    settings_.sessionCommandSources);
	results.reserve(commands.size());
	for (const SessionCommand &command : commands)
		results.push_back(dispatcher.dispatch(command));
	return results;
}

std::vector<InventoryScriptRunResult> RuntimeSourceDrainer::drainInventoryScripts()
{
	std::vector<InventoryScriptRunResult> results;
	if (!session_.hasActiveWorld())
		return results;

	std::vector<std::filesystem::path> paths = RuntimeSourceStream<std::filesystem::path, InventoryScriptSource> {}.drain(
	    settings_.inventoryScriptSources);
	results.reserve(paths.size());
	for (const std::filesystem::path &path : paths)
		results.push_back(runInventoryScript(path));
	return results;
}

std::vector<InventoryCommandResult> RuntimeSourceDrainer::drainInventoryCommands()
{
	std::vector<InventoryCommandResult> results;
	if (!session_.hasActiveWorld())
		return results;

	std::vector<InventoryCommand> commands = RuntimeSourceStream<InventoryCommand, InventoryCommandSource> {}.drain(
	    settings_.inventoryCommandSources);
	results.reserve(commands.size());
	if (settings_.inputPlayerId >= session_.world().players.size()) {
		for (const InventoryCommand &command : commands) {
			results.push_back({ .type = InventoryCommandResultType::Rejected, .command = command });
			inventoryEvents_.emit({
			    .type = InventoryEventType::Rejected,
			    .commandType = command.type,
			    .commandResult = InventoryCommandResultType::Rejected,
			});
		}
		return results;
	}

	InventoryCommandDispatcher dispatcher { session_.world().players[settings_.inputPlayerId], &inventoryEvents_ };
	for (const InventoryCommand &command : commands)
		results.push_back(dispatcher.dispatch(command));

	return results;
}

std::vector<MovementScriptRunResult> RuntimeSourceDrainer::drainMovementScripts()
{
	std::vector<MovementScriptRunResult> results;
	if (!session_.hasActiveWorld())
		return results;

	std::vector<std::filesystem::path> paths = RuntimeSourceStream<std::filesystem::path, MovementScriptSource> {}.drain(
	    settings_.movementScriptSources);
	results.reserve(paths.size());

	for (const std::filesystem::path &path : paths)
		results.push_back(runMovementScript(path));

	return results;
}

int RuntimeSourceDrainer::drainMovementCommands()
{
	if (!session_.hasActiveWorld())
		return 0;

	int queued = 0;
	std::vector<MovementCommand> commands = RuntimeSourceStream<MovementCommand, MovementCommandSource> {}.drain(
	    routedMovementCommands_,
	    settings_.movementCommandSources);
	for (MovementCommand command : commands) {
		session_.world().commandQueue.push(command);
		++queued;
	}
	return queued;
}

} // namespace dev
