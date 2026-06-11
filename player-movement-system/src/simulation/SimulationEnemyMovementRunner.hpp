#pragma once

#include "player/Player.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationEnemyMovementRunner {
public:
	void run(SimulationWorld &world, Player &target, float deltaSeconds) const;
};

} // namespace dev
