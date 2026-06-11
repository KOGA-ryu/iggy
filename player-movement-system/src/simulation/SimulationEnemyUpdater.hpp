#pragma once

#include "simulation/SimulationEnemyMovementRunner.hpp"
#include "simulation/SimulationEnemyTargetSelector.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationEnemyUpdater {
public:
	void update(SimulationWorld &world, float deltaSeconds) const;

private:
	SimulationEnemyMovementRunner movementRunner_;
	SimulationEnemyTargetSelector targetSelector_;
};

} // namespace dev
