#pragma once

#include "events/MovementEventSink.hpp"
#include "interaction/DestinationAction.hpp"
#include "player/Player.hpp"
#include "world/Collision.hpp"
#include "world/PathFinder.hpp"
#include "world/TileMap.hpp"

namespace dev {

class PlayerPathPlanner {
public:
	PlayerPathPlanner(const TileMap &map, const Collision &collision, const PathFinder &pathFinder, MovementEventSink *eventSink = nullptr);

	void walkTo(Player &player, PlayerId playerId, Point destination) const;
	void moveThenAct(Player &player, PlayerId playerId, Point destination, DestinationAction action) const;

private:
	void emit(MovementEventType type, PlayerId playerId, Point tile, DestinationActionType actionType = DestinationActionType::None) const;

	const TileMap &map_;
	const Collision &collision_;
	const PathFinder &pathFinder_;
	MovementEventSink *eventSink_;
};

} // namespace dev
