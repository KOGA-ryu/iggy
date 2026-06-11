#pragma once

#include <vector>

#include "commands/MovementCommand.hpp"
#include "player/Player.hpp"
#include "world/PathFinder.hpp"
#include "world/TileMap.hpp"

namespace dev {

class PlayerController {
public:
	PlayerController(std::vector<Player> &players, const TileMap &map, const Collision &collision, const PathFinder &pathFinder);

	void walkTo(PlayerId playerId, Point destination);
	void moveThenAct(PlayerId playerId, Point destination, DestinationAction action);
	void standAndAct(PlayerId playerId, DestinationAction action);
	void stop(PlayerId playerId);

private:
	std::vector<Player> &players_;
	const TileMap &map_;
	const Collision &collision_;
	const PathFinder &pathFinder_;
};

} // namespace dev
