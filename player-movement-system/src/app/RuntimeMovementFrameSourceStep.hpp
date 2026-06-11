#pragma once

#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSourceDrainer.hpp"

namespace dev {

class RuntimeMovementFrameSourceStep {
public:
	void run(
	    RuntimeSourceDrainer &sourceDrainer,
	    RuntimeRunRecorder &recorder) const;
};

} // namespace dev
