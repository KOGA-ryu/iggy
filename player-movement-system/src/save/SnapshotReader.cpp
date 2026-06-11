#include "SnapshotReader.hpp"

namespace dev {

void SnapshotReader::read(const SimulationSnapshot &snapshot, SimulationWorld &world) const
{
	world.players = snapshot.players;
	world.enemies = snapshot.enemies;
	world.combat.registry().replaceAll(snapshot.combatants);
}

} // namespace dev
