#pragma once

#include <vector>

#include "combat/Combatant.hpp"
#include "enemies/Enemy.hpp"
#include "items/Item.hpp"
#include "player/Player.hpp"
#include "targeting/Target.hpp"

namespace dev {

struct SimulationSnapshot {
	std::vector<Player> players;
	std::vector<Enemy> enemies;
	std::vector<Item> items;
	std::vector<Combatant> combatants;
	std::vector<Target> targets;
};

} // namespace dev
