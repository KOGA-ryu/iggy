#include "GameLoop.hpp"

#include <cstddef>
#include <utility>

namespace dev {

namespace {

std::vector<SessionEvent> EventsSince(const SessionEventRecorder &recorder, std::size_t offset)
{
	const std::vector<SessionEvent> &events = recorder.events();
	if (offset >= events.size())
		return {};
	return { events.begin() + static_cast<std::ptrdiff_t>(offset), events.end() };
}

std::vector<InventoryEvent> EventsSince(const InventoryEventRecorder &recorder, std::size_t offset)
{
	const std::vector<InventoryEvent> &events = recorder.events();
	if (offset >= events.size())
		return {};
	return { events.begin() + static_cast<std::ptrdiff_t>(offset), events.end() };
}

void AppendInventoryCommandResults(std::vector<InventoryCommandResult> &out, const std::vector<InventoryScriptRunResult> &scriptResults)
{
	for (const InventoryScriptRunResult &scriptResult : scriptResults) {
		out.insert(
		    out.end(),
		    scriptResult.commandResults.begin(),
		    scriptResult.commandResults.end());
	}
}

} // namespace

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
	RuntimeSourceDrainer drainer {
		session_,
		inventoryEvents_,
		routedSessionCommands_,
		routedMovementCommands_,
		{
		    .sessionCommandSources = settings_.sessionCommandSources,
		    .inventoryScriptSources = settings_.inventoryScriptSources,
		    .inventoryCommandSources = settings_.inventoryCommandSources,
		    .movementCommandSources = settings_.movementCommandSources,
		    .inputPlayerId = settings_.inputPlayerId,
		},
	};

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
		result.inventoryScriptResult = drainer.runInventoryScript(*settings_.inventoryScript);
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
		const std::size_t sessionEventOffset = sessionEvents_.events().size();
		const std::size_t inventoryEventOffset = inventoryEvents_.events().size();
		RuntimeFrameReport frameReport;

		frameReport.rawInputEventsRouted = routeRawInputSources();
		result.rawInputEventsRouted += frameReport.rawInputEventsRouted;

		frameReport.sessionCommandResults = drainer.drainSessionCommands(dispatcher);
		result.sessionCommandResults.insert(result.sessionCommandResults.end(), frameReport.sessionCommandResults.begin(), frameReport.sessionCommandResults.end());

		frameReport.inventoryScriptResults = drainer.drainInventoryScripts();
		result.runtimeInventoryScriptResults.insert(
		    result.runtimeInventoryScriptResults.end(),
		    frameReport.inventoryScriptResults.begin(),
		    frameReport.inventoryScriptResults.end());
		AppendInventoryCommandResults(frameReport.inventoryCommandResults, frameReport.inventoryScriptResults);
		AppendInventoryCommandResults(result.inventoryCommandResults, frameReport.inventoryScriptResults);

		std::vector<InventoryCommandResult> inventoryResults = drainer.drainInventoryCommands();
		frameReport.inventoryCommandResults.insert(frameReport.inventoryCommandResults.end(), inventoryResults.begin(), inventoryResults.end());
		result.inventoryCommandResults.insert(result.inventoryCommandResults.end(), inventoryResults.begin(), inventoryResults.end());

		frameReport.movementCommandsQueued = drainer.drainMovementCommands();
		result.movementCommandsQueued += frameReport.movementCommandsQueued;

		frameReport.frameEvents = updateSimulationFrame();
		result.lastFrameEvents = frameReport.frameEvents;
		frameReport.sessionEvents = EventsSince(sessionEvents_, sessionEventOffset);
		frameReport.inventoryEvents = EventsSince(inventoryEvents_, inventoryEventOffset);
		result.frameReports.push_back(frameReport);

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

SimulationFrameEvents GameLoop::updateSimulationFrame()
{
	return session_.update(settings_.fixedDeltaSeconds);
}

void GameLoop::renderDebugView() {}

} // namespace dev
