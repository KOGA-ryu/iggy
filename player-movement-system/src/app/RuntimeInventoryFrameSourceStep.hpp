#pragma once

#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSourceDrainer.hpp"

namespace dev {

class RuntimeInventoryFrameSourceStep {
public:
	void run(
	    RuntimeSourceDrainer &sourceDrainer,
	    RuntimeRunRecorder &recorder) const;
};

} // namespace dev
