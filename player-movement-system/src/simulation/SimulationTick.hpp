#pragma once

#include "simulation/SimulationCommandDrainer.hpp"
#include "simulation/SimulationFramePolicy.hpp"
#include "simulation/SimulationPlayerUpdater.hpp"
#include "simulation/SimulationTimeStep.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationTick {
public:
	void update(SimulationWorld &world, float deltaSeconds) const;
	void update(SimulationWorld &world, float deltaSeconds, const SimulationFramePolicy &policy) const;
	void update(SimulationWorld &world, const SimulationTimeStep &timeStep) const;
	void update(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const;

private:
	SimulationCommandDrainer commands_;
	SimulationPlayerUpdater players_;
};

} // namespace dev
