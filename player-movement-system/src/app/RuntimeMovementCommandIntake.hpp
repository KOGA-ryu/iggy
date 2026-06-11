#pragma once

#include <vector>

#include "commands/MovementCommand.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class RuntimeMovementCommandIntake {
public:
	[[nodiscard]] int queue(
	    std::vector<MovementCommand> commands,
	    SimulationWorld &world) const;
};

} // namespace dev
