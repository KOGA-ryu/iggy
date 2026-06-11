#pragma once

#include "app/RuntimeFrameLoopRunner.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunFinalizer.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSetupRunner.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeRunExecutor {
public:
	RuntimeRunExecutor(
	    GameSession &session,
	    RuntimeRunRecorder &recorder,
	    RuntimeSetupRunner &setupRunner,
	    RuntimeFrameLoopRunner &frameLoopRunner,
	    RuntimeRunFinalizer finalizer = RuntimeRunFinalizer {});

	void run(
	    GameLoopResult &result,
	    const RuntimeSetupSettings &setup,
	    const RuntimeOutputSettings &output) const;

private:
	GameSession &session_;
	RuntimeRunRecorder &recorder_;
	RuntimeSetupRunner &setupRunner_;
	RuntimeFrameLoopRunner &frameLoopRunner_;
	RuntimeRunFinalizer finalizer_;
};

} // namespace dev
