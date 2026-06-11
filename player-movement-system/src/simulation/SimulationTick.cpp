#include "SimulationTick.hpp"

#include "actions/ActionExecutor.hpp"
#include "commands/CommandDispatcher.hpp"
#include "enemies/EnemyMovement.hpp"
#include "player/PlayerController.hpp"
#include "player/PlayerMovement.hpp"

namespace dev {

void SimulationTick::update(SimulationWorld &world, float deltaSeconds) const
{
	dispatchQueuedCommands(world);

	ActionExecutor actionExecutor { ActionRules {}, world.movementEvents, &world.combat };
	PlayerMovement playerMovement { world.collision, actionExecutor, world.movementEvents };
	playerMovement.update(world.players, deltaSeconds);

	if (!world.players.empty()) {
		EnemyMovement enemyMovement { world.map, world.collision, world.movementEvents, &world.combat };
		enemyMovement.update(world.enemies, world.players.front(), deltaSeconds);
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

