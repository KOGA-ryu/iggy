#include "RuntimeFrameRunner.hpp"

#include "app/RuntimeInputContextBuilder.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeRawInputDrainer.hpp"

namespace dev {

RuntimeFrameRunner::RuntimeFrameRunner(
    GameSession &session,
    QueuedSessionCommandSource &routedSessionCommands,
    QueuedMovementCommandSource &routedMovementCommands,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder,
    SessionCommandDispatcher &sessionDispatcher,
    const RuntimeSourceSettings &sources,
    const RuntimeInputSettings &input,
    const RuntimeFrameSettings &frame)
    : session_(session)
    , routedSessionCommands_(routedSessionCommands)
    , routedMovementCommands_(routedMovementCommands)
    , sourceDrainer_(sourceDrainer)
    , recorder_(recorder)
    , sessionDispatcher_(sessionDispatcher)
    , sources_(sources)
    , input_(input)
    , frame_(frame)
{
}

void RuntimeFrameRunner::runFrame()
{
	recorder_.beginFrame();

	recorder_.recordRawInputEventsRouted(routeRawInputSources());

	recorder_.recordSessionCommandResults(sourceDrainer_.drainSessionCommands(sessionDispatcher_));

	recorder_.recordInventoryScriptResults(sourceDrainer_.drainInventoryScripts());
	recorder_.recordInventoryCommandResults(sourceDrainer_.drainInventoryCommands());

	recorder_.recordMovementCommandsQueued(sourceDrainer_.drainMovementCommands());

	recorder_.recordFrameEvents(updateSimulationFrame());
	recorder_.finishFrame();

	renderDebugView();
}

int RuntimeFrameRunner::routeRawInputSources()
{
	RuntimeInputRouter router { routedSessionCommands_, routedMovementCommands_, input_.bindings };
	RuntimeRawInputDrainer drainer { router };
	return drainer.drain(sources_.rawInputSources, RuntimeInputContextBuilder { session_ }.build(input_));
}

SimulationFrameEvents RuntimeFrameRunner::updateSimulationFrame()
{
	return session_.update(frame_.fixedDeltaSeconds);
}

void RuntimeFrameRunner::renderDebugView() {}

} // namespace dev
