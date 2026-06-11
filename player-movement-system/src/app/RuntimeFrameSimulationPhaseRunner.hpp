#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeFrameSimulationPhaseRunner {
public:
	void run(
	    GameSession &session,
	    const RuntimeFrameSettings &frame,
	    RuntimeRunRecorder &recorder) const;
};

} // namespace dev
