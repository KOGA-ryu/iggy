#include "RuntimeSimulationFrameUpdater.hpp"

namespace dev {

SimulationFrameEvents RuntimeSimulationFrameUpdater::update(
    GameSession &session,
    const RuntimeFrameSettings &frame) const
{
	return session.update(frame.fixedDeltaSeconds);
}

} // namespace dev
