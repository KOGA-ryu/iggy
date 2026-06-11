#include "RuntimeSourceDrainer.hpp"

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

std::vector<SessionCommandResult> RuntimeSourceDrainer::drainSessionCommands(const SessionCommandDispatcher &dispatcher)
{
	std::vector<SessionCommandResult> results;
	std::vector<SessionCommandSource *> sources;
	sources.reserve(settings_.sessionCommandSources.size() + 1U);
	sources.push_back(&routedSessionCommands_);
	sources.insert(sources.end(), settings_.sessionCommandSources.begin(), settings_.sessionCommandSources.end());

	for (SessionCommandSource *source : sources) {
		if (source == nullptr)
			continue;
		std::vector<SessionCommand> commands = source->drain();
		results.reserve(results.size() + commands.size());
		for (const SessionCommand &command : commands)
			results.push_back(dispatcher.dispatch(command));
	}
	return results;
}

std::vector<InventoryScriptRunResult> RuntimeSourceDrainer::drainInventoryScripts()
{
	std::vector<InventoryScriptRunResult> results;
	if (!session_.hasActiveWorld())
		return results;

	for (InventoryScriptSource *source : settings_.inventoryScriptSources) {
		if (source == nullptr)
			continue;
		std::vector<std::filesystem::path> paths = source->drain();
		results.reserve(results.size() + paths.size());
		for (const std::filesystem::path &path : paths)
			results.push_back(runInventoryScript(path));
	}
	return results;
}

std::vector<InventoryCommandResult> RuntimeSourceDrainer::drainInventoryCommands()
{
	std::vector<InventoryCommandResult> results;
	if (!session_.hasActiveWorld())
		return results;

	for (InventoryCommandSource *source : settings_.inventoryCommandSources) {
		if (source == nullptr)
			continue;
		std::vector<InventoryCommand> commands = source->drain();
		results.reserve(results.size() + commands.size());
		if (settings_.inputPlayerId >= session_.world().players.size()) {
			for (const InventoryCommand &command : commands) {
				results.push_back({ .type = InventoryCommandResultType::Rejected, .command = command });
				inventoryEvents_.emit({
				    .type = InventoryEventType::Rejected,
				    .commandType = command.type,
				    .commandResult = InventoryCommandResultType::Rejected,
				});
			}
			continue;
		}

		InventoryCommandDispatcher dispatcher { session_.world().players[settings_.inputPlayerId], &inventoryEvents_ };
		for (const InventoryCommand &command : commands)
			results.push_back(dispatcher.dispatch(command));
	}

	return results;
}

int RuntimeSourceDrainer::drainMovementCommands()
{
	if (!session_.hasActiveWorld())
		return 0;

	int queued = 0;
	std::vector<MovementCommandSource *> sources;
	sources.reserve(settings_.movementCommandSources.size() + 1U);
	sources.push_back(&routedMovementCommands_);
	sources.insert(sources.end(), settings_.movementCommandSources.begin(), settings_.movementCommandSources.end());

	for (MovementCommandSource *source : sources) {
		if (source == nullptr)
			continue;
		std::vector<MovementCommand> commands = source->drain();
		for (MovementCommand command : commands) {
			session_.world().commandQueue.push(command);
			++queued;
		}
	}
	return queued;
}

} // namespace dev
