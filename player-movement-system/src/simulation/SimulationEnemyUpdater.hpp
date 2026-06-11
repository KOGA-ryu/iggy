#pragma once

#include "simulation/SimulationEnemyTargetSelector.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationEnemyUpdater {
public:
	void update(SimulationWorld &world, float deltaSeconds) const;

private:
	SimulationEnemyTargetSelector targetSelector_;
};

} // namespace dev
