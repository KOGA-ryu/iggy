#include "SimulationTick.hpp"

#include "actions/ActionExecutor.hpp"
#include "commands/CommandDispatcher.hpp"
#include "enemies/EnemyMovement.hpp"
#include "player/PlayerController.hpp"
#include "player/PlayerMovement.hpp"

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
		dispatchQueuedCommands(world);
	}

	if (policy.updatePlayers && timeStep.playerDeltaSeconds > 0.0F) {
		ActionExecutor actionExecutor { ActionRules {}, world.movementEvents, &world.combat };
		PlayerMovement playerMovement { world.collision, actionExecutor, world.movementEvents };
		playerMovement.update(world.players, timeStep.playerDeltaSeconds);
	}

	if (policy.updateEnemies && timeStep.enemyDeltaSeconds > 0.0F && !world.players.empty()) {
		EnemyMovement enemyMovement { world.map, world.collision, world.movementEvents, &world.combat };
		enemyMovement.update(world.enemies, world.players.front(), timeStep.enemyDeltaSeconds);
	}
}

void SimulationTick::dispatchQueuedCommands(SimulationWorld &world) const
{
	PlayerController playerController { world.players, world.map, world.collision, world.pathFinder, world.movementEvents };
	CommandDispatcher dispatcher { playerController, world.movementEvents };

	MovementCommand command {};
	while (world.commandQueue.tryPop(command)) {
		dispatcher.dispatch(command);
	}
}

} // namespace dev
