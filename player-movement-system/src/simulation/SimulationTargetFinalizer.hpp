#pragma once

#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationWorld.hpp"
#include "targeting/TargetSynchronizer.hpp"

namespace dev {

class SimulationTargetFinalizer {
public:
	void finalize(SimulationWorld &world, const SimulationFrameEvents &frameEvents) const;

private:
	TargetSynchronizer targets_;
};

} // namespace dev
