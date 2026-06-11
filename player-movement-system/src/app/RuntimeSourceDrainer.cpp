#include "RuntimeSourceDrainer.hpp"

#include "app/RuntimeInventoryCommandIntake.hpp"
#include "app/RuntimeInventoryScriptBatchRunner.hpp"
#include "app/RuntimeInventoryScriptIntake.hpp"
#include "app/RuntimeMovementCommandIntake.hpp"
#include "app/RuntimeMovementScriptBatchRunner.hpp"
#include "app/RuntimeMovementScriptIntake.hpp"
#include "app/RuntimeSessionCommandIntake.hpp"
#include "app/RuntimeSourceContext.hpp"
#include "app/RuntimeSourceStream.hpp"

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
	RuntimeSourceContext context { session_, settings_.inputPlayerId };
	return RuntimeInventoryScriptIntake {}.run(
	    path,
	    context.hasActivePlayer() ? &context.player() : nullptr,
	    &inventoryEvents_);
}

MovementScriptRunResult RuntimeSourceDrainer::runMovementScript(const std::filesystem::path &path)
{
	RuntimeSourceContext context { session_, settings_.inputPlayerId };
	return RuntimeMovementScriptIntake {}.run(
	    path,
	    context.hasActiveWorld() ? &context.world() : nullptr);
}

std::vector<SessionCommandResult> RuntimeSourceDrainer::drainSessionCommands(const SessionCommandDispatcher &dispatcher)
{
	std::vector<SessionCommand> commands = RuntimeSourceStream<SessionCommand, SessionCommandSource> {}.drain(
	    routedSessionCommands_,
	    settings_.sessionCommandSources);
	return RuntimeSessionCommandIntake {}.dispatch(std::move(commands), dispatcher);
}

std::vector<InventoryScriptRunResult> RuntimeSourceDrainer::drainInventoryScripts()
{
	RuntimeSourceContext context { session_, settings_.inputPlayerId };
	if (!context.hasActiveWorld())
		return {};

	std::vector<std::filesystem::path> paths = RuntimeSourceStream<std::filesystem::path, InventoryScriptSource> {}.drain(
	    settings_.inventoryScriptSources);
	return RuntimeInventoryScriptBatchRunner {}.run(
	    std::move(paths),
	    context.hasActivePlayer() ? &context.player() : nullptr,
	    &inventoryEvents_);
}

std::vector<InventoryCommandResult> RuntimeSourceDrainer::drainInventoryCommands()
{
	std::vector<InventoryCommandResult> results;
	RuntimeSourceContext context { session_, settings_.inputPlayerId };
	if (!context.hasActiveWorld())
		return results;

	std::vector<InventoryCommand> commands = RuntimeSourceStream<InventoryCommand, InventoryCommandSource> {}.drain(
	    settings_.inventoryCommandSources);
	return RuntimeInventoryCommandIntake {}.dispatch(
	    std::move(commands),
	    context.hasActivePlayer() ? &context.player() : nullptr,
	    &inventoryEvents_);
}

std::vector<MovementScriptRunResult> RuntimeSourceDrainer::drainMovementScripts()
{
	RuntimeSourceContext context { session_, settings_.inputPlayerId };
	if (!context.hasActiveWorld())
		return {};

	std::vector<std::filesystem::path> paths = RuntimeSourceStream<std::filesystem::path, MovementScriptSource> {}.drain(
	    settings_.movementScriptSources);
	return RuntimeMovementScriptBatchRunner {}.run(std::move(paths), context.world());
}

int RuntimeSourceDrainer::drainMovementCommands()
{
	RuntimeSourceContext context { session_, settings_.inputPlayerId };
	if (!context.hasActiveWorld())
		return 0;

	std::vector<MovementCommand> commands = RuntimeSourceStream<MovementCommand, MovementCommandSource> {}.drain(
	    routedMovementCommands_,
	    settings_.movementCommandSources);
	return RuntimeMovementCommandIntake {}.queue(std::move(commands), context.world());
}

} // namespace dev
