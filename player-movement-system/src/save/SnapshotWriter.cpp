#include "SnapshotWriter.hpp"

namespace dev {

SimulationSnapshot SnapshotWriter::write(const SimulationWorld &world) const
{
	return {
	    .players = world.players,
	    .enemies = world.enemies,
	    .items = world.items,
	    .combatants = world.combat.registry().combatants(),
	    .targets = world.targets.targets(),
	};
}

} // namespace dev
