#pragma once

#include "simulation/SimulationCommandQueueDrainStep.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationCommandDrainer {
public:
	[[nodiscard]] int drain(SimulationWorld &world) const;

private:
	SimulationCommandQueueDrainStep queueDrain_;
};

} // namespace dev
