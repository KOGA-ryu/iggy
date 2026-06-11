#pragma once

#include "simulation/SimulationFramePolicyDescriber.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeFramePolicyResolver {
public:
	[[nodiscard]] SimulationFramePolicyDescription resolve(const GameSession &session) const;
};

} // namespace dev
