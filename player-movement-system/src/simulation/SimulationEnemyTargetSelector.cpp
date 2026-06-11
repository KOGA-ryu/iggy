#include "SimulationEnemyTargetSelector.hpp"

namespace dev {

Player *SimulationEnemyTargetSelector::selectTarget(SimulationWorld &world) const
{
	if (world.players.empty())
		return nullptr;
	return &world.players.front();
}

} // namespace dev
