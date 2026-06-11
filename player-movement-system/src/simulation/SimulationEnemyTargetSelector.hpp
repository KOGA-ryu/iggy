#pragma once

#include "player/Player.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationEnemyTargetSelector {
public:
	[[nodiscard]] Player *selectTarget(SimulationWorld &world) const;
};

} // namespace dev
