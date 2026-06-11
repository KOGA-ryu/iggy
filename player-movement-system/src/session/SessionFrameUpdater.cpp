#include "SessionFrameUpdater.hpp"

#include "session/SessionModePolicy.hpp"
#include "simulation/SimulationFrameRunner.hpp"

namespace dev {

SimulationFrameEvents SessionFrameUpdater::update(
    SimulationWorld &world,
    SimulationClock &clock,
    GameSessionMode mode,
    float rawDeltaSeconds) const
{
	SessionModePolicy policy;
	if (!policy.hasActiveWorld(mode))
		return {};

	SimulationFrameRunner runner { &clock };
	return runner.run(world, rawDeltaSeconds, policy.framePolicyFor(mode));
}

} // namespace dev
