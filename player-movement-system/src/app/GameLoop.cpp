#include "GameLoop.hpp"

#include "RuntimeInputRouter.hpp"
#include "RuntimeExitCodePolicy.hpp"
#include "RuntimeOutputFinalizer.hpp"
#include "RuntimeRawInputDrainer.hpp"
#include "RuntimeRunRecorder.hpp"
#include "RuntimeSetupRunner.hpp"
#include "RuntimeSourceDrainer.hpp"
#include "session/SessionCommandDispatcher.hpp"

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
	return RuntimeExitCodePolicy {}.exitCodeFor(result);
}

GameLoopResult GameLoop::runForResult()
{
	GameLoopResult result;
	RuntimeRunRecorder recorder { result, sessionEvents_, inventoryEvents_ };
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

	RuntimeSetupRunResult setup = RuntimeSetupRunner { dispatcher, drainer }.run(settings_.setup);
	result.setup = setup.setup;
	result.summary.inventoryCommandResults.insert(
	    result.summary.inventoryCommandResults.end(),
	    setup.inventoryCommandResults.begin(),
	    setup.inventoryCommandResults.end());
	if (!setup.framesAllowed)
		return finish();

	for (int frame = 0; frame < settings_.frame.maxFrames; ++frame) {
		recorder.beginFrame();

		recorder.recordRawInputEventsRouted(routeRawInputSources());

		recorder.recordSessionCommandResults(drainer.drainSessionCommands(dispatcher));

		recorder.recordInventoryScriptResults(drainer.drainInventoryScripts());
		recorder.recordInventoryCommandResults(drainer.drainInventoryCommands());

		recorder.recordMovementCommandsQueued(drainer.drainMovementCommands());

		recorder.recordFrameEvents(updateSimulationFrame());
		recorder.finishFrame();

		renderDebugView();
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
	RuntimeRawInputDrainer drainer { router };
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
	return drainer.drain(settings_.sources.rawInputSources, context);
}

SimulationFrameEvents GameLoop::updateSimulationFrame()
{
	return session_.update(settings_.frame.fixedDeltaSeconds);
}

void GameLoop::renderDebugView() {}

} // namespace dev
