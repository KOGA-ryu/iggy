#include "SimulationTimeStep.hpp"

#include <algorithm>

namespace dev {

SimulationTimeStep SimulationTimeStep::fromRawDelta(float deltaSeconds)
{
	const float clampedDelta = std::max(0.0F, deltaSeconds);
	return fromActorDelta(clampedDelta, clampedDelta);
}

SimulationTimeStep SimulationTimeStep::fromActorDelta(float rawDeltaSeconds, float actorDeltaSeconds)
{
	const float clampedRawDelta = std::max(0.0F, rawDeltaSeconds);
	const float clampedActorDelta = std::max(0.0F, actorDeltaSeconds);
	return {
	    .rawDeltaSeconds = clampedRawDelta,
	    .playerDeltaSeconds = clampedActorDelta,
	    .enemyDeltaSeconds = clampedActorDelta,
	    .animationDeltaSeconds = clampedRawDelta,
	};
}

} // namespace dev
