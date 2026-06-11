#include "SimulationFrameTickRunner.hpp"

#include "simulation/SimulationFrameEventCapture.hpp"

namespace dev {

SimulationFrameEvents SimulationFrameTickRunner::run(
    SimulationWorld &world,
    const SimulationTimeStep &timeStep,
    const SimulationFramePolicy &policy) const
{
	SimulationFrameEventCapture capture { world };
	tick_.update(world, timeStep, policy);
	capture.restore();
	return capture.events();
}

} // namespace dev
