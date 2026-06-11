#include "GameLoop.hpp"

#include "RuntimeInputRouter.hpp"
#include "RuntimeExitCodePolicy.hpp"
#include "RuntimeOutputFinalizer.hpp"
#include "RuntimeSourceDrainer.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionScriptRunner.hpp"

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
	return RuntimeExitCodePolicy {}.exitCodeFor(result);
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
		    .sessionCommandSources = settings_.sources.sessionCommandSources,
		    .inventoryScriptSources = settings_.sources.inventoryScriptSources,
		    .inventoryCommandSources = settings_.sources.inventoryCommandSources,
		    .movementCommandSources = settings_.sources.movementCommandSources,
		    .inputPlayerId = settings_.input.playerId,
		},
	};

	auto finish = [&]() {
		result.finalMode = session_.mode();
		RuntimeOutputFinalizer {}.finalize(settings_.output, result);
		return result;
	};

	if (settings_.setup.startupScript.has_value()) {
		result.setup.startupScriptRan = true;
		SessionScriptRunner runner { dispatcher };
		result.setup.startupScriptResult = runner.run(*settings_.setup.startupScript);
		if (result.setup.startupScriptResult.status == SessionScriptRunStatus::LoadFailed)
			return finish();
	}

	if (settings_.setup.inventoryScript.has_value()) {
		result.setup.inventoryScriptRan = true;
		result.setup.inventoryScriptResult = drainer.runInventoryScript(*settings_.setup.inventoryScript);
		if (result.setup.inventoryScriptResult.status != InventoryScriptRunStatus::Completed)
			return finish();
		result.summary.inventoryCommandResults.insert(
		    result.summary.inventoryCommandResults.end(),
		    result.setup.inventoryScriptResult.commandResults.begin(),
		    result.setup.inventoryScriptResult.commandResults.end());
	}

	for (int frame = 0; frame < settings_.frame.maxFrames; ++frame) {
		const std::size_t sessionEventOffset = sessionEvents_.events().size();
		const std::size_t inventoryEventOffset = inventoryEvents_.events().size();
		RuntimeFrameReport frameReport;

		frameReport.rawInputEventsRouted = routeRawInputSources();
		result.summary.rawInputEventsRouted += frameReport.rawInputEventsRouted;

		frameReport.sessionCommandResults = drainer.drainSessionCommands(dispatcher);
		result.summary.sessionCommandResults.insert(result.summary.sessionCommandResults.end(), frameReport.sessionCommandResults.begin(), frameReport.sessionCommandResults.end());

		frameReport.inventoryScriptResults = drainer.drainInventoryScripts();
		result.summary.runtimeInventoryScriptResults.insert(
		    result.summary.runtimeInventoryScriptResults.end(),
		    frameReport.inventoryScriptResults.begin(),
		    frameReport.inventoryScriptResults.end());
		AppendInventoryCommandResults(frameReport.inventoryCommandResults, frameReport.inventoryScriptResults);
		AppendInventoryCommandResults(result.summary.inventoryCommandResults, frameReport.inventoryScriptResults);

		std::vector<InventoryCommandResult> inventoryResults = drainer.drainInventoryCommands();
		frameReport.inventoryCommandResults.insert(frameReport.inventoryCommandResults.end(), inventoryResults.begin(), inventoryResults.end());
		result.summary.inventoryCommandResults.insert(result.summary.inventoryCommandResults.end(), inventoryResults.begin(), inventoryResults.end());

		frameReport.movementCommandsQueued = drainer.drainMovementCommands();
		result.summary.movementCommandsQueued += frameReport.movementCommandsQueued;

		frameReport.frameEvents = updateSimulationFrame();
		result.summary.lastFrameEvents = frameReport.frameEvents;
		frameReport.sessionEvents = EventsSince(sessionEvents_, sessionEventOffset);
		frameReport.inventoryEvents = EventsSince(inventoryEvents_, inventoryEventOffset);
		result.frameReports.push_back(frameReport);

		renderDebugView();
		++result.summary.framesRun;
	}

	return finish();
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
	RuntimeInputRouter router { routedSessionCommands_, routedMovementCommands_, settings_.input.bindings };
	RuntimeInputContext context {
		.world = session_.hasActiveWorld() ? &session_.world() : nullptr,
		.playerId = settings_.input.playerId,
		.focusState = settings_.input.focusState,
		.actionContext = settings_.input.actionContext,
		.sessionMode = session_.mode(),
		.targetResolver = settings_.input.targetResolver != nullptr
		    ? settings_.input.targetResolver
		    : (session_.hasActiveWorld() ? &session_.world().targets : nullptr),
	};

	int routed = 0;
	for (RawInputSource *source : settings_.sources.rawInputSources) {
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
	return session_.update(settings_.frame.fixedDeltaSeconds);
}

void GameLoop::renderDebugView() {}

} // namespace dev
