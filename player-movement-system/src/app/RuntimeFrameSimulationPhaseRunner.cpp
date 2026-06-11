#include "RuntimeFrameSimulationPhaseRunner.hpp"

#include "app/RuntimeFramePolicyResolver.hpp"
#include "app/RuntimeSimulationFrameUpdater.hpp"

namespace dev {

void RuntimeFrameSimulationPhaseRunner::run(
    GameSession &session,
    const RuntimeFrameSettings &frame,
    RuntimeRunRecorder &recorder) const
{
	recorder.recordFramePolicy(RuntimeFramePolicyResolver {}.resolve(session));
	recorder.recordFrameEvents(RuntimeSimulationFrameUpdater {}.update(session, frame));
}

} // namespace dev
