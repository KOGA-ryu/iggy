#pragma once

#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationFramePolicy.hpp"
#include "simulation/SimulationTick.hpp"
#include "simulation/SimulationTimeStep.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationFrameTickRunner {
public:
	[[nodiscard]] SimulationFrameEvents run(
	    SimulationWorld &world,
	    const SimulationTimeStep &timeStep,
	    const SimulationFramePolicy &policy) const;

private:
	SimulationTick tick_;
};

} // namespace dev
