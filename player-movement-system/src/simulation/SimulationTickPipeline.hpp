#pragma once

#include "simulation/SimulationActorUpdater.hpp"
#include "simulation/SimulationCommandDrainer.hpp"
#include "simulation/SimulationFramePolicy.hpp"
#include "simulation/SimulationTimeStep.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationTickPipeline {
public:
	void run(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const;

private:
	SimulationCommandDrainer commands_;
	SimulationActorUpdater actors_;
};

} // namespace dev
