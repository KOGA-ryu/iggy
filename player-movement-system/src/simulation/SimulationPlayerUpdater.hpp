#pragma once

#include "simulation/SimulationPlayerMovementRunner.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationPlayerUpdater {
public:
	void update(SimulationWorld &world, float deltaSeconds) const;

private:
	SimulationPlayerMovementRunner movementRunner_;
};

} // namespace dev
