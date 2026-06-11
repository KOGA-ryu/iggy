#pragma once

#include "session/GameSessionMode.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SessionFrameUpdater {
public:
	[[nodiscard]] SimulationFrameEvents update(
	    SimulationWorld &world,
	    SimulationClock &clock,
	    GameSessionMode mode,
	    float rawDeltaSeconds) const;
};

} // namespace dev
