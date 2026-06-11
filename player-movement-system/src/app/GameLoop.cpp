#include "GameLoop.hpp"

#include <utility>

namespace dev {

GameLoop::GameLoop(GameLoopSettings settings)
    : settings_(std::move(settings))
    , session_(settings_.saveRoot)
{
}

int GameLoop::run()
{
	const GameLoopResult result = runForResult();
	if (result.startupScriptRan && result.startupScriptResult.status == SessionScriptRunStatus::LoadFailed)
		return 1;
	if (result.inventoryScriptRan && result.inventoryScriptResult.status != InventoryScriptRunStatus::Completed)
		return 1;
	return 0;
}

GameLoopResult GameLoop::runForResult()
{
	GameLoopResult result;
	SessionCommandDispatcher dispatcher { session_, &sessionEvents_ };

	if (settings_.startupScript.has_value()) {
		result.startupScriptRan = true;
		SessionScriptRunner runner { dispatcher };
		result.startupScriptResult = runner.run(*settings_.startupScript);
		if (result.startupScriptResult.status == SessionScriptRunStatus::LoadFailed) {
			result.finalMode = session_.mode();
			return result;
		}
	}

	if (settings_.inventoryScript.has_value()) {
		result.inventoryScriptRan = true;
		result.inventoryScriptResult = runInventoryScript(*settings_.inventoryScript);
		if (result.inventoryScriptResult.status != InventoryScriptRunStatus::Completed) {
			result.finalMode = session_.mode();
			return result;
		}
		result.inventoryCommandResults.insert(
		    result.inventoryCommandResults.end(),
		    result.inventoryScriptResult.commandResults.begin(),
		    result.inventoryScriptResult.commandResults.end());
	}

	for (int frame = 0; frame < settings_.maxFrames; ++frame) {
		result.rawInputEventsRouted += routeRawInputSources();
		std::vector<SessionCommandResult> drained = drainSessionCommandSources(dispatcher);
		result.sessionCommandResults.insert(result.sessionCommandResults.end(), drained.begin(), drained.end());
		std::vector<InventoryScriptRunResult> inventoryScriptResults = drainInventoryScriptSources();
		result.runtimeInventoryScriptResults.insert(
		    result.runtimeInventoryScriptResults.end(),
		    inventoryScriptResults.begin(),
		    inventoryScriptResults.end());
		for (const InventoryScriptRunResult &scriptResult : inventoryScriptResults) {
			result.inventoryCommandResults.insert(
			    result.inventoryCommandResults.end(),
			    scriptResult.commandResults.begin(),
			    scriptResult.commandResults.end());
		}
		std::vector<InventoryCommandResult> inventoryResults = drainInventoryCommandSources();
		result.inventoryCommandResults.insert(result.inventoryCommandResults.end(), inventoryResults.begin(), inventoryResults.end());
		result.movementCommandsQueued += drainMovementCommandSources();
		result.lastFrameEvents = updateSimulationFrame();
		renderDebugView();
		++result.framesRun;
	}

	result.finalMode = session_.mode();
	return result;
}

GameSession &GameLoop::session()
{
	return session_;
}

const GameSession &GameLoop::session() const
{
	return session_;
}

const SessionEventRecorder &GameLoop::sessionEvents() const
{
	return sessionEvents_;
}

const InventoryEventRecorder &GameLoop::inventoryEvents() const
{
	return inventoryEvents_;
}

InventoryScriptRunResult GameLoop::runInventoryScript(const std::filesystem::path &path)
{
	if (!session_.hasActiveWorld() || settings_.inputPlayerId >= session_.world().players.size())
		return { .status = InventoryScriptRunStatus::NoActivePlayer };

	InventoryCommandDispatcher dispatcher { session_.world().players[settings_.inputPlayerId], &inventoryEvents_ };
	InventoryScriptRunner runner { dispatcher };
	return runner.run(path);
}

int GameLoop::routeRawInputSources()
{
	RuntimeInputRouter router { routedSessionCommands_, routedMovementCommands_, settings_.inputBindings };
	RuntimeInputContext context {
		.world = session_.hasActiveWorld() ? &session_.world() : nullptr,
		.playerId = settings_.inputPlayerId,
		.focusState = settings_.inputFocusState,
		.actionContext = settings_.playerActionContext,
		.sessionMode = session_.mode(),
		.targetResolver = settings_.targetResolver != nullptr
		    ? settings_.targetResolver
		    : (session_.hasActiveWorld() ? &session_.world().targets : nullptr),
	};

	int routed = 0;
	for (RawInputSource *source : settings_.rawInputSources) {
		if (source == nullptr)
			continue;
		std::vector<RawInputEvent> events = source->drain();
		for (const RawInputEvent &event : events) {
			RuntimeInputRouteResult result = router.route(event, context);
			if (result.handled)
				++routed;
		}
	}
	return routed;
}

std::vector<SessionCommandResult> GameLoop::drainSessionCommandSources(const SessionCommandDispatcher &dispatcher)
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
		for (const SessionCommand &command : commands) {
			results.push_back(dispatcher.dispatch(command));
		}
	}
	return results;
}

std::vector<InventoryScriptRunResult> GameLoop::drainInventoryScriptSources()
{
	std::vector<InventoryScriptRunResult> results;
	if (!session_.hasActiveWorld())
		return results;

	std::vector<InventoryScriptSource *> sources = settings_.inventoryScriptSources;
	for (InventoryScriptSource *source : sources) {
		if (source == nullptr)
			continue;
		std::vector<std::filesystem::path> paths = source->drain();
		results.reserve(results.size() + paths.size());
		for (const std::filesystem::path &path : paths)
			results.push_back(runInventoryScript(path));
	}
	return results;
}

std::vector<InventoryCommandResult> GameLoop::drainInventoryCommandSources()
{
	std::vector<InventoryCommandResult> results;
	if (!session_.hasActiveWorld())
		return results;

	std::vector<InventoryCommandSource *> sources = settings_.inventoryCommandSources;
	for (InventoryCommandSource *source : sources) {
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

int GameLoop::drainMovementCommandSources()
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

SimulationFrameEvents GameLoop::updateSimulationFrame()
{
	return session_.update(settings_.fixedDeltaSeconds);
}

void GameLoop::renderDebugView() {}

} // namespace dev
