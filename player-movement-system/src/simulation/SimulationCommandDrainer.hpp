#pragma once

#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationCommandDrainer {
public:
	void drain(SimulationWorld &world) const;
};

} // namespace dev
