#include "EnemyPursuitStepGate.hpp"

namespace dev {

EnemyPursuitStepGate::EnemyPursuitStepGate(const TileMap &map, const Collision &collision)
    : map_(map)
    , collision_(collision)
{
}

bool EnemyPursuitStepGate::canEnter(Point tile) const
{
	return map_.isWalkable(tile) && !collision_.blocksMovement(tile);
}

} // namespace dev
