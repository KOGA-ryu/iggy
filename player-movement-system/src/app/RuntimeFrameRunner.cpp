#include "RuntimeFrameRunner.hpp"

#include "app/RuntimeFrameSimulationPhaseRunner.hpp"
#include "app/RuntimeFrameSourcePhaseRunner.hpp"

namespace dev {

RuntimeFrameRunner::RuntimeFrameRunner(
    GameSession &session,
    RuntimeInputSourceRouter &inputSourceRouter,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder,
    SessionCommandDispatcher &sessionDispatcher,
    const RuntimeFrameSettings &frame)
    : session_(session)
    , inputSourceRouter_(inputSourceRouter)
    , sourceDrainer_(sourceDrainer)
    , recorder_(recorder)
    , sessionDispatcher_(sessionDispatcher)
    , frame_(frame)
{
}

void RuntimeFrameRunner::runFrame()
{
	recorder_.beginFrame();

	RuntimeFrameSourcePhaseRunner {}.run(
	    inputSourceRouter_,
	    sourceDrainer_,
	    recorder_,
	    sessionDispatcher_);

	RuntimeFrameSimulationPhaseRunner {}.run(session_, frame_, recorder_);
	recorder_.finishFrame();

	renderDebugView();
}

void RuntimeFrameRunner::renderDebugView() {}

} // namespace dev
