#pragma once

#include "app/RuntimeInputSourceRouter.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "session/SessionCommandDispatcher.hpp"

namespace dev {

class RuntimeSessionFrameSourceStep {
public:
	void run(
	    RuntimeInputSourceRouter &inputSourceRouter,
	    RuntimeSourceDrainer &sourceDrainer,
	    RuntimeRunRecorder &recorder,
	    const SessionCommandDispatcher &sessionDispatcher) const;
};

} // namespace dev
