#pragma once

#include "simulation/SimulationFramePolicy.hpp"
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
	void dispatchQueuedCommands(SimulationWorld &world) const;
};

} // namespace dev
