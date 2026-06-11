#pragma once

#include <vector>

#include "combat/Combatant.hpp"
#include "enemies/Enemy.hpp"
#include "player/Player.hpp"

namespace dev {

struct SimulationSnapshot {
	std::vector<Player> players;
	std::vector<Enemy> enemies;
	std::vector<Combatant> combatants;
};

} // namespace dev
