#include "SimulationTick.hpp"

#include "enemies/EnemyMovement.hpp"

namespace dev {

void SimulationTick::update(SimulationWorld &world, float deltaSeconds) const
{
	update(world, SimulationTimeStep::fromRawDelta(deltaSeconds));
}

void SimulationTick::update(SimulationWorld &world, float deltaSeconds, const SimulationFramePolicy &policy) const
{
	update(world, SimulationTimeStep::fromRawDelta(deltaSeconds), policy);
}

void SimulationTick::update(SimulationWorld &world, const SimulationTimeStep &timeStep) const
{
	update(world, timeStep, SimulationFramePolicy::forMode(SimulationMode::Gameplay));
}

void SimulationTick::update(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const
{
	if (policy.acceptCommands) {
		commands_.drain(world);
	}

	if (policy.updatePlayers)
		players_.update(world, timeStep.playerDeltaSeconds);

	if (policy.updateEnemies && timeStep.enemyDeltaSeconds > 0.0F && !world.players.empty()) {
		EnemyMovement enemyMovement { world.map, world.collision, world.movementEvents, &world.combat };
		enemyMovement.update(world.enemies, world.players.front(), timeStep.enemyDeltaSeconds);
	}
}

} // namespace dev
