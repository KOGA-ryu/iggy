#pragma once

#include "session/GameSessionMode.hpp"
#include "simulation/SimulationFramePolicy.hpp"

namespace dev {

class SessionModePolicy {
public:
	[[nodiscard]] bool hasActiveWorld(GameSessionMode mode) const;
	[[nodiscard]] bool canTransition(GameSessionMode current, GameSessionMode requested) const;
	[[nodiscard]] SimulationFramePolicy framePolicyFor(GameSessionMode mode) const;
};

} // namespace dev
