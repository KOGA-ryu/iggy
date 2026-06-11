#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeSimulationFrameUpdater {
public:
	[[nodiscard]] SimulationFrameEvents update(
	    GameSession &session,
	    const RuntimeFrameSettings &frame) const;
};

} // namespace dev
