#include "NewGameWorldBuilder.hpp"

namespace dev {

SimulationWorld NewGameWorldBuilder::build(
    const NewGameSettings &settings,
    MovementEventSink *movementEvents,
    CombatEventSink *combatEvents) const
{
	SimulationWorld world;
	world.movementEvents = movementEvents;
	world.setCombatEventSink(combatEvents);
	world.players.push_back(buildPlayer(settings));
	return world;
}

Player NewGameWorldBuilder::buildPlayer(const NewGameSettings &settings) const
{
	Player player;
	player.position.tile = settings.playerStart;
	player.position.future = settings.playerStart;
	player.position.previous = settings.playerStart;
	player.position.precise = settings.playerStart;
	player.combatStats.hitPoints = settings.playerHitPoints;
	return player;
}

} // namespace dev
