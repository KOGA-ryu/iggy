#include "ActionRules.hpp"

#include <algorithm>
#include <cstdlib>

namespace dev {

namespace {

int ChebyshevDistance(Point a, Point b)
{
	return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

} // namespace

bool ActionRules::canExecute(const Player &player, const DestinationAction &action) const
{
	if (action.type == DestinationActionType::None)
		return false;
	if (player.moveState == PlayerMoveState::Stunned)
		return false;
	if (!player.animationLock.canCancel())
		return false;
	return true;
}

bool ActionRules::targetInRange(const Player &player, const DestinationAction &action) const
{
	return ChebyshevDistance(player.position.tile, action.target.tile) <= action.rangeTiles;
}

bool ActionRules::targetStillValid(const DestinationAction &action) const
{
	return action.type != DestinationActionType::None;
}

} // namespace dev

