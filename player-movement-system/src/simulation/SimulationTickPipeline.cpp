#include "SimulationTickPipeline.hpp"

namespace dev {

void SimulationTickPipeline::run(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const
{
	if (policy.acceptCommands) {
		commands_.drain(world);
	}

	actors_.update(world, timeStep, policy);
}

} // namespace dev
