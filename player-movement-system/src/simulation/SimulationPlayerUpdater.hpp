#pragma once

#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationPlayerUpdater {
public:
	void update(SimulationWorld &world, float deltaSeconds) const;
};

} // namespace dev
