#pragma once

#include <vector>

#include "combat/CombatEventSink.hpp"
#include "combat/CombatSystem.hpp"
#include "commands/CommandQueue.hpp"
#include "enemies/Enemy.hpp"
#include "events/MovementEventSink.hpp"
#include "player/Player.hpp"
#include "world/Collision.hpp"
#include "world/PathFinder.hpp"
#include "world/TileMap.hpp"

namespace dev {

struct SimulationWorld {
	TileMap map;
	Collision collision;
	PathFinder pathFinder;
	CommandQueue commandQueue;
	std::vector<Player> players;
	std::vector<Enemy> enemies;
	MovementEventSink *movementEvents = nullptr;
	CombatEventSink *combatEvents = nullptr;
	CombatSystem combat { combatEvents };

	void setCombatEventSink(CombatEventSink *eventSink);
};

} // namespace dev

