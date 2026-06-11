#pragma once

#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationPlayerMovementRunner {
public:
	void run(SimulationWorld &world, float deltaSeconds) const;
};

} // namespace dev
