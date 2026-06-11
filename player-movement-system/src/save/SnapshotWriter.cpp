#include "SnapshotWriter.hpp"

namespace dev {

SimulationSnapshot SnapshotWriter::write(const SimulationWorld &world) const
{
	return {
	    .players = world.players,
	    .enemies = world.enemies,
	    .combatants = world.combat.registry().combatants(),
	};
}

} // namespace dev
