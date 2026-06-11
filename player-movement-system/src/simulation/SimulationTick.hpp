#pragma once

#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationTick {
public:
	void update(SimulationWorld &world, float deltaSeconds) const;

private:
	void dispatchQueuedCommands(SimulationWorld &world) const;
};

} // namespace dev

