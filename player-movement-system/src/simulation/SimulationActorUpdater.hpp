#pragma once

#include "simulation/SimulationEnemyUpdater.hpp"
#include "simulation/SimulationFramePolicy.hpp"
#include "simulation/SimulationPlayerUpdater.hpp"
#include "simulation/SimulationTimeStep.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationActorUpdater {
public:
	void update(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const;

private:
	SimulationPlayerUpdater players_;
	SimulationEnemyUpdater enemies_;
};

} // namespace dev
