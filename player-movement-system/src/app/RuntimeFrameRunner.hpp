#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeInputSourceRouter.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"

namespace dev {

class RuntimeFrameRunner {
public:
	RuntimeFrameRunner(
	    GameSession &session,
	    RuntimeInputSourceRouter &inputSourceRouter,
	    RuntimeSourceDrainer &sourceDrainer,
	    RuntimeRunRecorder &recorder,
	    SessionCommandDispatcher &sessionDispatcher,
	    const RuntimeFrameSettings &frame);

	void runFrame();

private:
	void renderDebugView();

	GameSession &session_;
	RuntimeInputSourceRouter &inputSourceRouter_;
	RuntimeSourceDrainer &sourceDrainer_;
	RuntimeRunRecorder &recorder_;
	SessionCommandDispatcher &sessionDispatcher_;
	const RuntimeFrameSettings &frame_;
};

} // namespace dev
